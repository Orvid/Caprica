#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <string_view>

#include <common/CapricaConfig.h>
#include <common/CapricaJobManager.h>
#include <common/CapricaReportingContext.h>
#include <common/CapricaStats.h>
#include <common/FakeScripts.h>
#include <common/FSUtils.h>
#include <common/GameID.h>
#include <common/parser/CapricaUserFlagsParser.h>

#include <papyrus/PapyrusCompilationContext.h>
#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusScript.h>
#include <papyrus/parser/PapyrusParser.h>

#include <pex/parser/PexAsmParser.h>
#include <pex/PexAsmWriter.h>
#include <pex/PexOptimizer.h>
#include <pex/PexReader.h>
#include <pex/PexWriter.h>

#include <Windows.h>

namespace conf = caprica::conf;
namespace FSUtils = caprica::FSUtils;
using caprica::pathEq;
using caprica::papyrus::PapyrusCompilationNode;

namespace caprica {
bool parseCommandLineArguments(int argc, char *argv[], caprica::CapricaJobManager *jobManager);

// A hack to speed up identifying the base game script directory
// Each of these are in the 
static caseless_unordered_identifier_ref_set starfieldBaseScriptDirSet = {
        "scriptobject",
        "form",
        "spellapplycameraattachedfxscript",
        "addfactiontolinkedrefontrigger"
};

static bool gBaseFound = true;

static caseless_unordered_identifier_ref_set fallout4BaseScriptDirSet = {
        "scriptobject",
        "form",
        "viciousdogfxscript"
};

// lower bound of the number of files in the root of the base script dir
constexpr size_t getBaseLowerFileCountLimit(GameID game) {
  switch (game) {
    case GameID::Fallout4:
      return 900;
    case GameID::Starfield:
      return 1400;
    default:
      return 0;
  }
}

caseless_unordered_identifier_ref_map<bool> getBaseSigMap(GameID game) {
  caseless_unordered_identifier_ref_map<bool> map{};
  switch(game){
    case GameID::Fallout4:
      map.reserve(fallout4BaseScriptDirSet.size());
      for (auto &id: fallout4BaseScriptDirSet)
        map.emplace(id, false);
      break;
    case GameID::Starfield:
      map.reserve(starfieldBaseScriptDirSet.size());
      for (auto &id: starfieldBaseScriptDirSet)
        map.emplace(id, false);
      break;
    default:
      break;
  }
  return map;
}

static const std::unordered_set FAKE_SKYRIM_SCRIPTS_SET = {
        "fake://skyrim/__ScriptObject.psc",
        "fake://skyrim/DLC1SCWispWallScript.psc",
};

bool handleImports(const std::vector<ImportFile>& f, caprica::CapricaJobManager* jobManager);

PapyrusCompilationNode* getNode(const PapyrusCompilationNode::NodeType& nodeType,
                                CapricaJobManager* jobManager,
                                const std::filesystem::path& baseOutputDir,
                                const std::filesystem::path& curDir,
                                const std::filesystem::path& absBaseDir,
                                const WIN32_FIND_DATA& data);

PapyrusCompilationNode* getNode(const PapyrusCompilationNode::NodeType& nodeType,
                                CapricaJobManager* jobManager,
                                const std::filesystem::path& baseOutputDir,
                                const std::filesystem::path& curDir,
                                const std::filesystem::path& absBaseDir,
                                const std::filesystem::path& fileName,
                                time_t lastModTime,
                                size_t fileSize);

bool addSingleFile(const InputFile& input,
                   const std::filesystem::path& baseOutputDir,
                   caprica::CapricaJobManager *jobManager,
                   PapyrusCompilationNode::NodeType nodeType);

bool addFilesFromDirectory(const InputFile& input,
                           const std::filesystem::path& baseOutputDir,
                           caprica::CapricaJobManager* jobManager,
                           PapyrusCompilationNode::NodeType nodeType,
                           const std::string& startingNS = "") {
  // Blargle flargle.... Using the raw Windows API is 5x
  // faster than boost::filesystem::recursive_directory_iterator,
  // at 40ms vs. 200ms for the boost solution, and the raw API
  // solution also gets us the relative paths, absolute paths,
  // last write time, and file size, all without any extra processing.
  if (!input.exists()) {
    std::cout << "ERROR: Input file '" << input.get_unresolved_path() << "' was not found!" << std::endl;
    return false;
  }
  auto absBaseDir = input.resolved_absolute_basedir();
  auto subdir = input.resolved_relative();
  if (subdir == ".")
    subdir = "";
  bool recursive = input.isRecursive();
  std::vector<std::filesystem::path> dirs {};
  dirs.push_back(subdir);
  auto baseDirMap = getBaseSigMap(conf::Papyrus::game);
  auto l_startNS = startingNS;
  const auto DOTDOT = std::string_view("..");
  const auto DOT = std::string_view(".");

  while (dirs.size()) {
    HANDLE hFind;
    WIN32_FIND_DATA data;
    auto curDir = dirs.back();
    dirs.pop_back();
    auto curSearchPattern = absBaseDir / curDir / "*";
    caprica::caseless_unordered_identifier_ref_map<PapyrusCompilationNode *> namespaceMap{};
    namespaceMap.reserve(8000);

    hFind = FindFirstFileA(curSearchPattern.string().c_str(), &data);
    if (hFind == INVALID_HANDLE_VALUE) {
      std::cout << "An error occurred while trying to iterate the files in '" << curSearchPattern << "'!" << std::endl;
      return false;
    }

    do {
      std::string_view filenameRef = data.cFileName;
      if (filenameRef != DOT && filenameRef != DOTDOT) {
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
          if (recursive)
            dirs.push_back(curDir / data.cFileName);
        } else {
          auto ext = FSUtils::extensionAsRef(filenameRef);
          bool skip = false;

          switch (nodeType) {
            case PapyrusCompilationNode::NodeType::PapyrusCompile:
            case PapyrusCompilationNode::NodeType::PapyrusImport:
              if (!pathEq(ext, ".psc"))
                skip = true;
              break;
            case PapyrusCompilationNode::NodeType::PasReflection:
            case PapyrusCompilationNode::NodeType::PasCompile:
              if (!pathEq(ext, ".pas"))
                skip = true;
              break;
            case PapyrusCompilationNode::NodeType::PexReflection:
            case PapyrusCompilationNode::NodeType::PexDissassembly:
              if (!pathEq(ext, ".pex"))
                skip = true;
              break;
            default:
              skip = true;
              break;
          }
          if (!skip) {
            PapyrusCompilationNode* node = getNode(nodeType, jobManager, baseOutputDir, curDir, absBaseDir, data);

            namespaceMap.emplace(caprica::identifier_ref(node->baseName), node);
            if (!gBaseFound && !baseDirMap.empty()) {
              // get the filename without the extension using substr assuming that the last 4 characters are the
              // extension
              if (baseDirMap.count(node->baseName) != 0)
                baseDirMap[node->baseName] = true;
            }
          }
        }
      }
    } while (FindNextFileA(hFind, &data));
    FindClose(hFind);

    if (conf::Papyrus::game > GameID::Skyrim) {
      if (!namespaceMap.empty()) {
        // check that all the values in the base script dir map are true
        if (!gBaseFound && !baseDirMap.empty()) {
          bool allTrue = true;
          for (auto& pair : baseDirMap) {
            if (!pair.second) {
              allTrue = false;
              break;
            }
          }
          if (allTrue && namespaceMap.size() >= getBaseLowerFileCountLimit(conf::Papyrus::game)) {
            // if it's true, then this is the base dir and the namespace should be root
            gBaseFound = true;
            l_startNS = "";
          }
        }
        // if it's true, then this is the base dir and the namespace should be root
        auto namespaceName = l_startNS + curDir.lexically_normal().string();
        std::replace(namespaceName.begin(), namespaceName.end(), '\\', ':');
        while (namespaceName[0] == ':')
          namespaceName = namespaceName.substr(1);
        caprica::papyrus::PapyrusCompilationContext::pushNamespaceFullContents(namespaceName, std::move(namespaceMap));
      }
    } else {
      caprica::papyrus::PapyrusCompilationContext::pushNamespaceFullContents("", std::move(namespaceMap));
    }
    // We don't repopulate it because it's either in the root of whatever was imported or it's not in this import at all
    baseDirMap.clear();
  }
  return true;
}

PapyrusCompilationNode* getNode(const PapyrusCompilationNode::NodeType& nodeType,
                                CapricaJobManager* jobManager,
                                const std::filesystem::path& baseOutputDir,
                                const std::filesystem::path& curDir,
                                const std::filesystem::path& absBaseDir,
                                const std::filesystem::path& fileName,
                                time_t lastModTime,
                                size_t fileSize) {
  std::filesystem::path cur;
  if (curDir == ".")
    cur = "";
  else
    cur = curDir;
  std::filesystem::path sourceFilePath = absBaseDir / cur / fileName;
  std::filesystem::path filenameToDisplay = cur / fileName;
  std::filesystem::path outputDir = baseOutputDir / cur;

  if (nodeType == PapyrusCompilationNode::NodeType::PapyrusImport ||
      nodeType == PapyrusCompilationNode::NodeType::PasReflection ||
      nodeType == PapyrusCompilationNode::NodeType::PexReflection) {
    CapricaStats::importedFileCount++;
  } else {
    CapricaStats::inputFileCount++;
  }
  auto node = new caprica::papyrus::PapyrusCompilationNode(jobManager,
                                                           nodeType,
                                                           std::move(filenameToDisplay.string()),
                                                           std::move(outputDir.string()),
                                                           std::move(sourceFilePath.string()),
                                                           lastModTime,
                                                           fileSize);
  return node;
}

PapyrusCompilationNode* getNode(const PapyrusCompilationNode::NodeType& nodeType,
                                CapricaJobManager* jobManager,
                                const std::filesystem::path& baseOutputDir,
                                const std::filesystem::path& curDir,
                                const std::filesystem::path& absBaseDir,
                                const WIN32_FIND_DATA& data) {
  const auto lastModTime = [](FILETIME ft) -> time_t {
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;
    return ull.QuadPart / 10000000ULL - 11644473600ULL;
  }(data.ftLastWriteTime);
  const auto fileSize = [](DWORD low, DWORD high) {
    ULARGE_INTEGER ull;
    ull.LowPart = low;
    ull.HighPart = high;
    return ull.QuadPart;
  }(data.nFileSizeLow, data.nFileSizeHigh);
  return getNode(nodeType, jobManager, baseOutputDir, curDir, absBaseDir, data.cFileName, lastModTime, fileSize);
}

bool handleImports(const std::vector<ImportFile>& f, caprica::CapricaJobManager* jobManager) {
  // Skyrim hacks; we need to import Skyrim's fake scripts into the global namespace first.
  if (conf::Papyrus::game == GameID::Skyrim) {
    caprica::caseless_unordered_identifier_ref_map<PapyrusCompilationNode *> tempMap{};
    for (auto &fake_script: FAKE_SKYRIM_SCRIPTS_SET) {
      auto basename = caprica::FSUtils::filenameAsRef(fake_script);

      auto node =
              new PapyrusCompilationNode(jobManager,
                                         PapyrusCompilationNode::NodeType::PapyrusImport,
                                         std::string(basename),
                                         "",
                                         std::string(fake_script),
                                         0,
                                         caprica::FakeScripts::getSizeOfFakeScript(fake_script, conf::Papyrus::game));
      tempMap.emplace(caprica::identifier_ref(node->baseName), node);
    }
    caprica::papyrus::PapyrusCompilationContext::pushNamespaceFullContents("", std::move(tempMap));
  }
  std::cout << "Importing files..." << std::endl;
  int i = 0;
  for (auto& dir : f) {
    std::string ns = "";
    if (conf::Papyrus::game > GameID::Skyrim)
      ns = "!!temp" + std::to_string(i++);
    if (!addFilesFromDirectory(dir, "", jobManager, PapyrusCompilationNode::NodeType::PapyrusImport, ns))
      return false;
  }
  CapricaStats::outputImportedCount();
  caprica::papyrus::PapyrusCompilationContext::RenameImports(jobManager);
  return true;
}

bool addSingleFile(const InputFile& input,
                   const std::filesystem::path& baseOutputDir,
                   caprica::CapricaJobManager* jobManager,
                   PapyrusCompilationNode::NodeType nodeType) {
  // Get the file size and last modified time using std::filesystem
  std::error_code ec;

  auto relPath = input.resolved_relative();
  auto namespaceDir = relPath.parent_path();
  auto absPath = input.resolved_absolute();
  auto filename = absPath.filename();
  auto absBaseDir = input.resolved_absolute_basedir();
  if (!input.exists()) {
    std::cout << "ERROR: Resolved file path '" << absPath
              << "' is not in an import directory, cannot resolve namespace!" << std::endl;
    return false;
  }
  auto lastModTime = std::filesystem::last_write_time(absPath, ec);
  if (ec) {
    std::cout << "An error occurred while trying to get the last modified time of '" << absPath << "'!" << std::endl;
    return false;
  }
  auto fileSize = std::filesystem::file_size(absPath, ec);
  if (ec) {
    std::cout << "An error occurred while trying to get the file size of '" << absPath << "'!" << std::endl;
    return false;
  }
  auto namespaceName = relPath.parent_path().string();
  std::replace(namespaceName.begin(), namespaceName.end(), '\\', ':');
  if (namespaceName[0] == ':')
    namespaceName = namespaceName.substr(1);
  if (!conf::General::quietCompile)
    std::cout << "Adding file '" << filename << "' to namespace '" << namespaceName << "'." << std::endl;
  auto node = getNode(nodeType,
                      jobManager,
                      baseOutputDir,
                      namespaceDir,
                      absBaseDir,
                      filename,
                      lastModTime.time_since_epoch().count(),
                      fileSize);
  caprica::papyrus::PapyrusCompilationContext::pushNamespaceFullContents(
          namespaceName,
          caprica::caseless_unordered_identifier_ref_map<PapyrusCompilationNode *>{
                  {caprica::identifier_ref(node->baseName), node}
          });
  return true;
}

void parseUserFlags(std::string &&flagsPath) {
  caprica::CapricaReportingContext reportingContext{flagsPath};
  auto parser = new caprica::parser::CapricaUserFlagsParser(reportingContext, flagsPath);
  parser->parseUserFlags(conf::Papyrus::userFlagsDefinition);
  delete parser;
}

}

int main(int argc, char *argv[]) {
  caprica::CapricaJobManager jobManager{};
  auto startParse = std::chrono::high_resolution_clock::now();
  if (!caprica::parseCommandLineArguments(argc, argv, &jobManager)) {
    caprica::CapricaReportingContext::breakIfDebugging();
    return -1;
  }
  if (conf::General::compileInParallel)
    jobManager.startup((uint32_t) std::thread::hardware_concurrency());
  auto endParse = std::chrono::high_resolution_clock::now();
  if (conf::Performance::dumpTiming) {
    std::cout << "Command Line Arg Parse: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(endParse - startParse).count() << "ms"
              << std::endl;
  }

  auto startRead = std::chrono::high_resolution_clock::now();
  if (conf::Performance::performanceTestMode)
    caprica::papyrus::PapyrusCompilationContext::awaitRead();
  auto endRead = std::chrono::high_resolution_clock::now();
  if (conf::Performance::dumpTiming) {
    std::cout << "Read: " << std::chrono::duration_cast<std::chrono::milliseconds>(endRead - startRead).count() << "ms"
              << std::endl;
  }

  try {
    auto startCompile = std::chrono::high_resolution_clock::now();
    caprica::papyrus::PapyrusCompilationContext::doCompile(&jobManager);
    auto endCompile = std::chrono::high_resolution_clock::now();
    if (conf::Performance::dumpTiming) {
      auto compTime = std::chrono::duration_cast<std::chrono::milliseconds>(endCompile - startCompile).count();
      std::cout << "Compiled "
                << "N/A" /*caprica::CapricaStats::inputFileCount*/ << " files in " << compTime << "ms" << std::endl;
      caprica::CapricaStats::outputStats();
    }
  } catch (const std::runtime_error &ex) {
    if (ex.what() != std::string(""))
      std::cout << ex.what() << std::endl;
    caprica::CapricaReportingContext::breakIfDebugging();
    return -1;
  }
  if (conf::Papyrus::game == caprica::GameID::Starfield) {
    std::cout << "**** WARNING! ****" << std::endl;
    std::cout << "The syntax for new features in Starfield (Guard, TryGuard, GetMatchingStructs) is experimental."
              << std::endl;
    std::cout << "It should be considered as unstable and subject to change." << std::endl << std::endl;
    std::cout << "The proper syntax will only be known when the Creation Kit comes out in early 2024," << std::endl;
    std::cout << "and subsequent releases of Caprica may drop support for this experimental syntax." << std::endl;
    std::cout << "Be prepared to update your scripts when the final syntax is known." << std::endl << std::endl;
  }

  jobManager.awaitShutdown();
  return 0;
}
