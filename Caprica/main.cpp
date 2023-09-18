#include <algorithm>
#include <chrono>
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
#include <common/parser/CapricaUserFlagsParser.h>

#include <papyrus/PapyrusCompilationContext.h>
#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusScript.h>
#include <papyrus/parser/PapyrusParser.h>

#include <pex/PexAsmWriter.h>
#include <pex/PexOptimizer.h>
#include <pex/PexReader.h>
#include <pex/PexWriter.h>
#include <pex/parser/PexAsmParser.h>

#include <Windows.h>

namespace conf = caprica::conf;
namespace FSUtils = caprica::FSUtils;
using caprica::pathEq;
using caprica::papyrus::PapyrusCompilationNode;

namespace caprica {
bool parseCommandLineArguments(int argc, char* argv[], caprica::CapricaJobManager* jobManager);


static const std::unordered_set FAKE_SKYRIM_SCRIPTS_SET = {
        "fake://skyrim/__ScriptObject.psc",
        "fake://skyrim/DLC1SCWispWallScript.psc",
};

bool addFilesFromDirectory(const std::string& f, bool recursive, const std::string& baseOutputDir, caprica::CapricaJobManager* jobManager) {
  // Blargle flargle.... Using the raw Windows API is 5x
  // faster than boost::filesystem::recursive_directory_iterator,
  // at 40ms vs. 200ms for the boost solution, and the raw API
  // solution also gets us the relative paths, absolute paths,
  // last write time, and filesize, all without any extra processing.
  auto absBaseDir = caprica::FSUtils::canonical(f);
  std::vector<std::string> dirs{ };
  dirs.push_back("\\");
  while (dirs.size()) {
    HANDLE hFind;
    WIN32_FIND_DATA data;
    auto curDir = dirs.back();
    dirs.pop_back();
    auto curSearchPattern = absBaseDir + curDir + "\\*";
    caprica::caseless_unordered_identifier_ref_map<PapyrusCompilationNode*> namespaceMap{ };

    // TODO: After imports are working, get rid of this hack
    if (conf::Papyrus::game == GameID::Skyrim){
      std::string outputDir = baseOutputDir;
      for (auto &fake_script : FAKE_SKYRIM_SCRIPTS_SET){
        auto basename = caprica::FSUtils::filenameAsRef(fake_script);

        auto node = new PapyrusCompilationNode(
                jobManager,
                PapyrusCompilationNode::NodeType::PapyrusImport,
                std::move(std::string(basename)),
                std::move(outputDir),
                std::move(std::string(fake_script)),
                0,
                caprica::FakeScripts::getSizeOfFakeScript(fake_script, conf::Papyrus::game)
        );
        namespaceMap.emplace(caprica::identifier_ref(node->baseName), node);
      }

    }
    namespaceMap.reserve(8000);
    std::string curDirFull;
    if (curDir == "\\")
      curDirFull = absBaseDir;
    else
      curDirFull = absBaseDir + curDir;

    hFind = FindFirstFileA(curSearchPattern.c_str(), &data);
    if (hFind == INVALID_HANDLE_VALUE) {
      std::cout << "An error occured while trying to iterate the files in '" << curSearchPattern << "'!" << std::endl;
      return false;
    }

    do {
      std::string_view filenameRef = data.cFileName;
      if (filenameRef != std::string_view(".") && filenameRef != std::string_view("..")) {
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
          if (recursive) {
            if (curDir == "\\")
              dirs.push_back(curDir + data.cFileName);
            else
              dirs.push_back(curDir + "\\" + data.cFileName);
          }
        } else {
          auto ext = FSUtils::extensionAsRef(filenameRef);
          if (pathEq(ext, ".psc")) {
            const auto calcLastModTime = [](FILETIME ft) -> time_t {
              ULARGE_INTEGER ull;
              ull.LowPart = ft.dwLowDateTime;
              ull.HighPart = ft.dwHighDateTime;
              return ull.QuadPart / 10000000ULL - 11644473600ULL;
            };
            std::string sourceFilePath = curDirFull + "\\" + data.cFileName;
            std::string filenameToDisplay;
            std::string outputDir;
            if (curDir == "\\") {
              filenameToDisplay = data.cFileName;
              outputDir = baseOutputDir;
            } else {
              filenameToDisplay = curDir.substr(1) + "\\" + data.cFileName;
              outputDir = baseOutputDir + curDir;
            }
            caprica::CapricaStats::inputFileCount++;
            auto node = new PapyrusCompilationNode(
              jobManager,
              PapyrusCompilationNode::NodeType::PapyrusCompile,
              std::move(filenameToDisplay),
              std::move(outputDir),
              std::move(sourceFilePath),
              calcLastModTime(data.ftLastWriteTime),
              [](DWORD low, DWORD high) {
                ULARGE_INTEGER ull;
                ull.LowPart = low;
                ull.HighPart = high;
                return ull.QuadPart;
              }(data.nFileSizeLow, data.nFileSizeHigh)
            );
            namespaceMap.emplace(caprica::identifier_ref(node->baseName), node);
          }
        }
      }
    } while (FindNextFileA(hFind, &data));
    FindClose(hFind);

    auto namespaceName = curDir;
    std::replace(namespaceName.begin(), namespaceName.end(), '\\', ':');
    namespaceName = namespaceName.substr(1);
    caprica::papyrus::PapyrusCompilationContext::pushNamespaceFullContents(namespaceName, std::move(namespaceMap));
  }
  return true;
}

void parseUserFlags(std::string&& flagsPath) {
  caprica::CapricaReportingContext reportingContext{ flagsPath };
  auto parser = new caprica::parser::CapricaUserFlagsParser(reportingContext, flagsPath);
  parser->parseUserFlags(conf::Papyrus::userFlagsDefinition);
  delete parser;
}

}

int main(int argc, char* argv[])
{
  caprica::CapricaJobManager jobManager{ };
  auto startParse = std::chrono::high_resolution_clock::now();
  if (!caprica::parseCommandLineArguments(argc, argv, &jobManager)) {
    caprica::CapricaReportingContext::breakIfDebugging();
    return -1;
  }
  if (conf::General::compileInParallel) {
    jobManager.startup((uint32_t)std::thread::hardware_concurrency());
  }
  auto endParse = std::chrono::high_resolution_clock::now();
  if (conf::Performance::dumpTiming)
    std::cout << "Command Line Arg Parse: " << std::chrono::duration_cast<std::chrono::milliseconds>(endParse - startParse).count() << "ms" << std::endl;

  auto startRead = std::chrono::high_resolution_clock::now();
  if (conf::Performance::performanceTestMode) {
    caprica::papyrus::PapyrusCompilationContext::awaitRead();
  }
  auto endRead = std::chrono::high_resolution_clock::now();
  if (conf::Performance::dumpTiming)
    std::cout << "Read: " << std::chrono::duration_cast<std::chrono::milliseconds>(endRead - startRead).count() << "ms" << std::endl;

  try {
    auto startCompile = std::chrono::high_resolution_clock::now();
    caprica::papyrus::PapyrusCompilationContext::doCompile(&jobManager);
    auto endCompile = std::chrono::high_resolution_clock::now();
    if (conf::Performance::dumpTiming) {
      auto compTime = std::chrono::duration_cast<std::chrono::milliseconds>(endCompile - startCompile).count();
      std::cout << "Compiled " << "N/A" /*caprica::CapricaStats::inputFileCount*/ << " files in " << compTime << "ms" << std::endl;
      caprica::CapricaStats::outputStats();
    }
  } catch (const std::runtime_error& ex) {
    if (ex.what() != std::string(""))
      std::cout << ex.what() << std::endl;
    caprica::CapricaReportingContext::breakIfDebugging();
    return -1;
  }

  jobManager.awaitShutdown();
  return 0;
}

