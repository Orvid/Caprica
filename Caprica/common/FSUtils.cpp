#include <common/FSUtils.h>

#include <concurrent_unordered_map.h>

#include <cstring>
#include <future>
#include <sstream>

#include <common/CapricaConfig.h>
#include <common/CapricaReportingContext.h>
#include <common/CaselessStringComparer.h>

namespace caprica { namespace FSUtils {

static Concurrency::concurrent_unordered_map<std::string, std::future<std::string>, CaselessStringHasher, CaselessStringEqual> futureFileReadMap{ };
static Concurrency::concurrent_unordered_map<std::string, std::string, CaselessStringHasher, CaselessStringEqual> readFilesMap{ };
std::string Cache::readFile(const std::string& filename) {
  std::ifstream inFile{ filename, std::ifstream::binary };
  std::stringstream strStream;
  strStream << inFile.rdbuf();
  auto str = strStream.str();
  readFilesMap.insert({ filename, str });
  return str;
}

void Cache::waitForAll() {
  for (auto& f : futureFileReadMap) {
    f.second.get();
  }
}

void Cache::push_need(const std::string& filename) {
  pushKnownExists(filename);
  if (conf::Performance::asyncFileRead) {
    auto abs = canonical(filename).string();
    if (!futureFileReadMap.count(abs)) {
      futureFileReadMap[abs] = std::async([abs]() {
        return readFile(abs);
      });
    }
  }
}

std::string Cache::cachedReadFull(const std::string& filename) {
  auto abs = canonical(filename).string();
  push_need(abs);
  if (readFilesMap.count(abs))
    return readFilesMap[abs];

  // If we're in performance test mode, all files should have been
  // discovered before starting to compile anything.
  if (conf::Performance::performanceTestMode)
    CapricaReportingContext::logicalFatal("Attempted to read a file at runtime in performance test mode.");

  if (!conf::Performance::asyncFileRead)
    return readFile(abs);

  // Doing wait is stupid expensive, and you can only call .get() once, so
  // we'd have to lock on the individual future, and it's cheaper to just
  // catch the exception when it's already been gotten once between when
  // we checked the map and when we get here.
  try {
    return futureFileReadMap[abs].get();
  } catch (...) {
    return readFilesMap[abs];
  }
}

static void writeFile(const std::string& filename, const std::string& value) {
  if (!conf::Performance::performanceTestMode) {
    std::ofstream destFile{ filename, std::ifstream::binary };
    destFile << value;
  }
}

void async_write(const std::string& filename, const std::string& value) {
  if (!conf::Performance::asyncFileWrite) {
    writeFile(filename, value);
  } else {
    std::async([](const std::string& filename, const std::string& value) {
      writeFile(filename, value);
    }, filename, value);
  }
}

static caseless_unordered_map<std::string, caseless_unordered_set<std::string>> directoryContentsMap{ };
void pushKnownInDirectory(const boost::filesystem::path& file) {
  auto dir = file.parent_path().string();
  if (!directoryContentsMap.count(dir)) {
    directoryContentsMap[dir] = { };
  }
  directoryContentsMap[dir].insert(file.string());
}

static Concurrency::concurrent_unordered_map<std::string, uint8_t, CaselessStringHasher, CaselessStringEqual> fileExistenceMap{ };
void pushKnownExists(const std::string& path) {
  fileExistenceMap.insert({ path, 2 });
}

bool exists(const std::string& path) {
  const auto checkAndSetExists = [](const std::string& path) {
    bool exists = false;
    auto f = directoryContentsMap.find(boost::filesystem::path(path).parent_path().string());
    if (f != directoryContentsMap.end()) {
      auto e = f->second.count(path);
      exists = e != 0;
    } else {
      exists = boost::filesystem::exists(path);
    }
    fileExistenceMap.insert({ path, exists ? 2 : 1 });
    return exists;
  };
  // These concurrent maps are a pain, as it's possible to get a value
  // in the process of being set.
  if (fileExistenceMap.count(path))
    return fileExistenceMap[path] == 2 ? true : checkAndSetExists(path);
  return checkAndSetExists(path);
}

// Borrowed and modified from http://stackoverflow.com/a/1750710/776797
boost::filesystem::path canonical(const boost::filesystem::path& path) {
  auto absPath = boost::filesystem::absolute(path);
  if (conf::Performance::resolveSymlinks) {
    return boost::filesystem::canonical(absPath).make_preferred();
  } else {
    boost::filesystem::path result;
    for (auto it = absPath.begin(); it != absPath.end(); ++it) {
      if (!wcscmp(it->c_str(), L"..")) {
        // /a/b/../.. is not /a/b/.. under most circumstances
        // We can end up with ..s in our result because of symbolic links
        if (!wcscmp(result.filename().c_str(), L".."))
          result /= *it;
        // Otherwise it should be safe to resolve the parent
        else
          result = result.parent_path();
      } else if (!wcscmp(it->c_str(), L".")) {
        // Ignore
      } else {
        // Just cat other path entries
        result /= *it;
      }
    }
    return result.make_preferred();
  }
}

// Borrowed and slightly modified from http://stackoverflow.com/a/5773008/776797
boost::filesystem::path naive_uncomplete(const boost::filesystem::path p, const boost::filesystem::path base) {
  using boost::filesystem::path;

  if (p == base)
    return L"./";

  /*!! this breaks stuff if path is a filename rather than a directory,
  which it most likely is... but then base shouldn't be a filename so... */
  boost::filesystem::path from_path, from_base, output;

  auto path_it = p.begin();
  const auto path_end = p.end();
  auto base_it = base.begin();
  const auto base_end = base.end();

  // check for emptiness
  if (path_it == path_end || base_it == base_end)
    throw std::runtime_error("path or base was empty; couldn't generate relative path");

#ifdef WIN32
  // drive letters are different; don't generate a relative path
  if (*path_it != *base_it)
    return p;

  // now advance past drive letters; relative paths should only go up
  // to the root of the drive and not past it
  ++path_it, ++base_it;
#endif

  // iterate over path and base
  while (true) {
    // compare all elements so far of path and base to find greatest common root;
    // when elements of path and base differ, or run out:
    if (path_it == path_end || base_it == base_end || *path_it != *base_it) {
      // write to output, ../ times the number of remaining elements in base;
      // this is how far we've had to come down the tree from base to get to the common root
      for (; base_it != base_end; ++base_it) {
        if (!wcscmp(base_it->c_str(), L"."))
          continue;
        else if (!wcscmp(base_it->c_str(), L"\\"))
          continue;

        output /= L"../";
      }

      // write to output, the remaining elements in path;
      // this is the path relative from the common root
      const auto path_it_start = path_it;
      for (; path_it != path_end; ++path_it) {

        if (path_it != path_it_start)
          output /= L"/";

        if (!wcscmp(base_it->c_str(), L"."))
          continue;
        if (!wcscmp(base_it->c_str(), L"\\"))
          continue;

        output /= *path_it;
      }

      break;
    }

    // add directory level to both paths and continue iteration
    from_path /= path(*path_it);
    from_base /= path(*base_it);

    ++path_it, ++base_it;
  }

  return output;
}

}}