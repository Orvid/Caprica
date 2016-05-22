#include <common/FSUtils.h>

#include <cstring>
#include <future>
#include <sstream>

#include <common/CapricaConfig.h>
#include <common/CapricaReportingContext.h>
#include <common/CaselessStringComparer.h>
#include <common/Concurrency.h>

namespace caprica { namespace FSUtils {

struct FileReadCacheEntry final
{
  FileReadCacheEntry() :
    read(std::make_unique<std::atomic<bool>>(false)),
    dataMutex(std::make_unique<std::mutex>()),
    taskMutex(std::make_unique<std::mutex>())
  {
  }
  FileReadCacheEntry(FileReadCacheEntry&& o) = default;
  FileReadCacheEntry& operator =(FileReadCacheEntry&&) = default;
  FileReadCacheEntry(const FileReadCacheEntry&) = delete;
  FileReadCacheEntry& operator =(const FileReadCacheEntry&) = delete;
  ~FileReadCacheEntry() = default;

  void wantFile(const std::string& filename, size_t filesize) {
    if (!this->filesize && filesize)
      this->filesize = filesize;
    if (conf::Performance::asyncFileRead) {
      if (!read->load() && !readTask.valid()) {
        std::unique_lock<std::mutex> lock{ *taskMutex };
        if (!read->load() && !readTask.valid()) {
          readTask = std::move(std::async(std::launch::async, [this, filename]() {
            this->readFile(filename);
          }));
        }
      }
    }
  }

  void readFile(const std::string& filename) {
    std::ifstream inFile{ filename, std::ifstream::binary };
    inFile.exceptions(std::ifstream::badbit | std::ifstream::failbit);
    std::string str;
    if (filesize != 0) {
      auto size = filesize;
      auto buf = new char[size];
      inFile.read(buf, size);
      str = std::string(buf, size);
      delete buf;
    }
    // Just because the filesize was one thing when
    // we iterated the directory doesn't mean it's
    // not gotten bigger since then.
    inFile.peek();
    if (!inFile.eof()) {
      std::stringstream strStream{ };
      strStream.exceptions(std::ifstream::badbit | std::ifstream::failbit);
      strStream << inFile.rdbuf();
      str += strStream.str();
    }
    {
      std::unique_lock<std::mutex> lock{ *dataMutex };
      readFileData = std::move(str);
      read->store(true);
    }
  }

  boost::string_ref getData(const std::string& filename) {
    if (read->load())
      return readFileData;

    if (readTask.valid()) {
      std::unique_lock<std::mutex> lock{ *taskMutex };
      if (readTask.valid())
        readTask.get();
      assert(read->load());
      return readFileData;
    }

    readFile(filename);
    assert(read->load());
    return readFileData;
  }

  void waitForRead() {
    if (readTask.valid()) {
      std::unique_lock<std::mutex> lock{ *taskMutex };
      if (readTask.valid())
        readTask.get();
      assert(read->load());
    }
  }

private:
  std::unique_ptr<std::atomic<bool>> read;
  std::unique_ptr<std::mutex> dataMutex;
  std::string readFileData{ "" };
  std::unique_ptr<std::mutex> taskMutex;
  std::future<void> readTask{ };
  size_t filesize{ 0 };
};

static caseless_concurrent_unordered_path_map<std::string, FileReadCacheEntry> readFilesMap{ };

void Cache::waitForAll() {
  for (auto& f : readFilesMap) {
    f.second.waitForRead();
  }
}

void Cache::push_need(const std::string& filename, size_t filesize) {
  pushKnownExists(filename);
  readFilesMap[filename].wantFile(filename, filesize);
}

boost::string_ref Cache::cachedReadFull(const std::string& filename) {
  auto abs = canonical(filename);
  push_need(abs);
  return readFilesMap[abs].getData(abs);
}

boost::string_ref basenameAsRef(boost::string_ref file) {
  auto pos = file.find_last_of("\\/");
  if (pos != boost::string_ref::npos)
    file = file.substr(pos + 1);
  auto pos2 = file.rfind('.');
  if (pos2 != boost::string_ref::npos)
    return file.substr(0, pos2);
  return file;
}

boost::string_ref extensionAsRef(boost::string_ref file) {
  auto pos = file.rfind('.');
  if (pos != boost::string_ref::npos)
    return file.substr(pos);
  return "";
}

boost::string_ref filenameAsRef(boost::string_ref file) {
  auto pos = file.find_last_of("\\/");
  if (pos != boost::string_ref::npos)
    return file.substr(pos + 1);
  return file;
}

boost::string_ref parentPathAsRef(boost::string_ref file) {
  auto pos = file.find_last_of("\\/");
  if (pos != boost::string_ref::npos)
    file = file.substr(0, pos);
  return file;
}

static void writeFile(const std::string& filename, std::string&& value) {
  if (!conf::Performance::performanceTestMode) {
    auto containingDir = boost::filesystem::path(filename).parent_path();
    if (!boost::filesystem::exists(containingDir))
      boost::filesystem::create_directories(containingDir);
    std::ofstream destFile{ filename, std::ifstream::binary };
    destFile.exceptions(std::ifstream::badbit | std::ifstream::failbit);
    destFile << value;
  }
}

void async_write(const std::string& filename, std::string&& value) {
  if (!conf::Performance::asyncFileWrite) {
    writeFile(filename, std::move(value));
  } else {
    std::async(std::launch::async, [](const std::string& filename, std::string&& value) {
      writeFile(filename, std::move(value));
    }, filename, std::move(value));
  }
}

static caseless_unordered_path_map<std::string, caseless_unordered_set<std::string>> directoryContentsMap{ };
void pushKnownInDirectory(const std::string& directory, caseless_unordered_set<std::string>&& files) {
  if (directoryContentsMap.count(directory))
    CapricaReportingContext::logicalFatal("Attempted to push the known directory state of '%s' multiple times!", directory.c_str());
  directoryContentsMap.emplace(directory, std::move(files));
}

static caseless_concurrent_unordered_path_map<std::string, uint8_t> fileExistenceMap{ };
void pushKnownExists(const std::string& path) {
  fileExistenceMap.insert({ path, 2 });
}

static bool checkAndSetExists(const std::string& path) {
  bool exists = false;
  auto toFind = boost::filesystem::path(path).parent_path().string();
  auto f = directoryContentsMap.find(toFind);
  if (f != directoryContentsMap.end()) {
    auto fName = filenameAsRef(path).to_string();
    auto e = f->second.count(fName);
    exists = e != 0;
  } else {
    exists = boost::filesystem::exists(path);
  }
  fileExistenceMap.insert({ path, exists ? 2 : 1 });
  return exists;
}

bool exists(const std::string& path) {
  // These concurrent maps are a pain, as it's possible to get a value
  // in the process of being set.
  if (fileExistenceMap.count(path))
    return fileExistenceMap[path] == 2 ? true : checkAndSetExists(path);
  return checkAndSetExists(path);
}

// Borrowed and modified from http://stackoverflow.com/a/1750710/776797
std::string canonical(const std::string& path) {
  if (path.size() > 3 && path[1] == ':') {
    // Shortcircuit for already canon paths.
    if (path.find("/") == std::string::npos &&
        path.find("..") == std::string::npos &&
        path.find("\\.\\") == std::string::npos &&
        path.find("\\\\") == std::string::npos) {
      return path;
    }
  }
  static boost::filesystem::path currentDirectory = boost::filesystem::current_path();
  auto absPath = boost::filesystem::absolute(path, currentDirectory);
  if (conf::Performance::resolveSymlinks) {
    return boost::filesystem::canonical(absPath).make_preferred().string();
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
    return result.make_preferred().string();
  }
}

}}
