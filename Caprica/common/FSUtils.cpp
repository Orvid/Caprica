#include <common/FSUtils.h>

#include <concurrent_unordered_map.h>
#include <future>

#include <common/CapricaConfig.h>
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
  if (CapricaConfig::asyncFileRead) {
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
  auto f = readFilesMap.find(abs);
  if (f != readFilesMap.end())
    return f->second;

  if (!CapricaConfig::asyncFileRead)
    return readFile(abs);

  return futureFileReadMap[abs].get();
}

static Concurrency::concurrent_unordered_map<std::string, std::future<void>, CaselessStringHasher, CaselessStringEqual> futureFileWriteMap{ };
static void writeFile(const std::string& filename, const std::string& value) {
  if (!CapricaConfig::performanceTestMode) {
    std::ofstream destFile{ filename, std::ifstream::binary };
    destFile << value;
  }
}

void async_write(const std::string& filename, const std::string& value) {
  if (!CapricaConfig::asyncFileWrite) {
    writeFile(filename, value);
  } else {
    futureFileWriteMap[filename] = std::async([](const std::string& filename, const std::string& value) {
      writeFile(filename, value);
    }, filename, value);
  }
}

// Borrowed and modified from http://stackoverflow.com/a/1750710/776797
boost::filesystem::path canonical(const boost::filesystem::path& path) {
  auto absPath = boost::filesystem::absolute(path);
  if (CapricaConfig::resolveSymlinks) {
    return boost::filesystem::canonical(absPath).make_preferred();
  } else {
    boost::filesystem::path result;
    for (auto it = absPath.begin(); it != absPath.end(); ++it) {
      if (*it == "..") {
        // /a/b/../.. is not /a/b/.. under most circumstances
        // We can end up with ..s in our result because of symbolic links
        if (result.filename() == "..")
          result /= *it;
        // Otherwise it should be safe to resolve the parent
        else
          result = result.parent_path();
      } else if (*it == ".") {
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
    return "./";
  /*!! this breaks stuff if path is a filename rather than a directory,
  which it most likely is... but then base shouldn't be a filename so... */

  boost::filesystem::path from_path, from_base, output;

  boost::filesystem::path::iterator path_it = p.begin(), path_end = p.end();
  boost::filesystem::path::iterator base_it = base.begin(), base_end = base.end();

  // check for emptiness
  if ((path_it == path_end) || (base_it == base_end))
    throw std::runtime_error("path or base was empty; couldn't generate relative path");

#ifdef WIN32
  // drive letters are different; don't generate a relative path
  if (*path_it != *base_it)
    return p;

  // now advance past drive letters; relative paths should only go up
  // to the root of the drive and not past it
  ++path_it, ++base_it;
#endif

  // Cache system-dependent dot, double-dot and slash strings
  const std::string _dot = ".";
  const std::string _dots = "..";
  const std::string _sep = "\\";

  // iterate over path and base
  while (true) {

    // compare all elements so far of path and base to find greatest common root;
    // when elements of path and base differ, or run out:
    if ((path_it == path_end) || (base_it == base_end) || (*path_it != *base_it)) {

      // write to output, ../ times the number of remaining elements in base;
      // this is how far we've had to come down the tree from base to get to the common root
      for (; base_it != base_end; ++base_it) {
        if (*base_it == _dot)
          continue;
        else if (*base_it == _sep)
          continue;

        output /= "../";
      }

      // write to output, the remaining elements in path;
      // this is the path relative from the common root
      boost::filesystem::path::iterator path_it_start = path_it;
      for (; path_it != path_end; ++path_it) {

        if (path_it != path_it_start)
          output /= "/";

        if (*path_it == _dot)
          continue;
        if (*path_it == _sep)
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