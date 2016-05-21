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
      str.append(buf, size);
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

  std::string getData(const std::string& filename) {
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

static concurrent_caseless_unordered_path_map<std::string, FileReadCacheEntry> readFilesMap{ };

void Cache::waitForAll() {
  for (auto& f : readFilesMap) {
    f.second.waitForRead();
  }
}

void Cache::push_need(const std::string& filename, size_t filesize) {
  pushKnownExists(filename);
  readFilesMap[filename].wantFile(filename, filesize);
}

std::string Cache::cachedReadFull(const std::string& filename) {
  auto abs = canonical(filename).string();
  push_need(abs);
  return readFilesMap[abs].getData(abs);
}

static void writeFile(const std::string& filename, const std::string& value) {
  if (!conf::Performance::performanceTestMode) {
    auto containingDir = boost::filesystem::path(filename).parent_path();
    if (!boost::filesystem::exists(containingDir))
      boost::filesystem::create_directories(containingDir);
    std::ofstream destFile{ filename, std::ifstream::binary };
    destFile.exceptions(std::ifstream::badbit | std::ifstream::failbit);
    destFile << value;
  }
}

void async_write(const std::string& filename, const std::string& value) {
  if (!conf::Performance::asyncFileWrite) {
    writeFile(filename, value);
  } else {
    std::async(std::launch::async, [](const std::string& filename, const std::string& value) {
      writeFile(filename, value);
    }, filename, value);
  }
}

static caseless_unordered_path_map<std::string, caseless_unordered_set<std::string>> directoryContentsMap{ };
void pushKnownInDirectory(const std::string& directory, caseless_unordered_set<std::string>&& files) {
  if (directoryContentsMap.count(directory))
    CapricaReportingContext::logicalFatal("Attempted to push the known directory state of '%s' multiple times!", directory.c_str());
  directoryContentsMap.insert({ directory, std::move(files) });
}

static concurrent_caseless_unordered_path_map<std::string, uint8_t> fileExistenceMap{ };
void pushKnownExists(const std::string& path) {
  fileExistenceMap.insert({ path, 2 });
}

const char* filenameAsRef(const std::string& file) {
  auto lSl = strrchr(file.c_str(), '\\');
  auto rSl = strrchr(file.c_str(), '/');
  if (lSl > rSl) {
    return lSl + 1;
  } else if (rSl != nullptr) {
    return rSl + 1;
  }
  return nullptr;
}

static bool checkAndSetExists(const std::string& path) {
  bool exists = false;
  auto toFind = boost::filesystem::path(path).parent_path().string();
  auto f = directoryContentsMap.find(toFind);
  if (f != directoryContentsMap.end()) {
    auto fName = filenameAsRef(path);
    auto e = f->second.count(fName);
    exists = e != 0;
  } else {
    exists = boost::filesystem::exists(path);
  }
  fileExistenceMap.insert({ path, exists ? 2 : 1 });
  return exists;
}

std::array<bool, 3> multiExistsInDir(const std::string& dir, std::array<std::string, 3>&& filenames) {
  auto f = directoryContentsMap.find(dir);
  if (f != directoryContentsMap.end()) {
    const auto fA = f->second.find(filenames[0]);
    const auto fB = f->second.find(filenames[1]);
    const auto fC = f->second.find(filenames[2]);
    return { fA != f->second.cend(), fB != f->second.cend(), fC != f->second.cend() };
  } else {
    return { exists(dir + "\\" + filenames[0]), exists(dir + "\\" + filenames[1]), exists(dir + "\\" + filenames[2]) };
  }
}

bool exists(const std::string& path) {
  // These concurrent maps are a pain, as it's possible to get a value
  // in the process of being set.
  if (fileExistenceMap.count(path))
    return fileExistenceMap[path] == 2 ? true : checkAndSetExists(path);
  return checkAndSetExists(path);
}

// Borrowed and modified from http://stackoverflow.com/a/1750710/776797
boost::filesystem::path canonical(const boost::filesystem::path& path) {
  static boost::filesystem::path currentDirectory = boost::filesystem::current_path();
  auto absPath = boost::filesystem::absolute(path, currentDirectory);
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

}}