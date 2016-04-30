#include <algorithm>
#include <chrono>
#include <fstream>
#include <ostream>
#include <string>

#include <ppl.h>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <common/CapricaConfig.h>
#include <common/CapricaReportingContext.h>
#include <common/FSUtils.h>
#include <common/parser/CapricaUserFlagsParser.h>

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

struct ScriptToCompile final
{
  boost::filesystem::path sourceFileName;
  std::string outputDirectory;

  ScriptToCompile() = delete;
  ScriptToCompile(boost::filesystem::path&& sourcePath,
                  const std::string& baseOutputDir,
                  boost::filesystem::path&& absolutePath,
                  boost::filesystem::path&& relOutputDir)
    : sourceFileName(std::move(sourcePath))
  {
    outputDirectory = baseOutputDir + "\\" + relOutputDir.string();
    caprica::FSUtils::Cache::push_need(absolutePath.string());
  }
  ScriptToCompile(boost::filesystem::path&& sourcePath, const std::string& baseOutputDir, boost::filesystem::path&& absolutePath)
    : sourceFileName(std::move(sourcePath)),
      outputDirectory(baseOutputDir)
  {
    caprica::FSUtils::Cache::push_need(absolutePath.string());
  }
  ~ScriptToCompile() = default;
};

static void compileScript(const ScriptToCompile& script) {
  if (!conf::General::quietCompile)
    std::cout << "Compiling " << script.sourceFileName << std::endl;
  auto path = boost::filesystem::path(script.sourceFileName);
  auto baseName = boost::filesystem::basename(path.filename());
  auto ext = boost::filesystem::extension(script.sourceFileName);
  caprica::CapricaReportingContext reportingContext{ script.sourceFileName.string() };
  if (!_stricmp(ext.c_str(), ".psc")) {
    auto parser = new caprica::papyrus::parser::PapyrusParser(reportingContext, script.sourceFileName.string());
    auto a = parser->parseScript();
    reportingContext.exitIfErrors();
    delete parser;
    auto ctx = new caprica::papyrus::PapyrusResolutionContext(reportingContext);
    a->preSemantic(ctx);
    a->semantic(ctx);
    reportingContext.exitIfErrors();
    auto pex = a->buildPex(reportingContext);
    reportingContext.exitIfErrors();
    delete ctx;
    delete a;

    if (conf::CodeGeneration::enableOptimizations)
      caprica::pex::PexOptimizer::optimize(pex);

    caprica::pex::PexWriter wtr{ };
    pex->write(wtr);
    wtr.writeToFile(script.outputDirectory + "\\" + baseName + ".pex");

    if (conf::Debug::dumpPexAsm) {
      std::ofstream asmStrm(script.outputDirectory + "\\" + baseName + ".pas", std::ofstream::binary);
      caprica::pex::PexAsmWriter asmWtr(asmStrm);
      pex->writeAsm(asmWtr);
    }

    delete pex;
  } else if (!_stricmp(ext.c_str(), ".pas")) {
    auto parser = new caprica::pex::parser::PexAsmParser(reportingContext, script.sourceFileName.string());
    auto pex = parser->parseFile();
    reportingContext.exitIfErrors();
    delete parser;

    if (conf::CodeGeneration::enableOptimizations)
      caprica::pex::PexOptimizer::optimize(pex);

    caprica::pex::PexWriter wtr{ };
    pex->write(wtr);
    wtr.writeToFile(script.outputDirectory + "\\" + baseName + ".pex");
    delete pex;
  } else if (!_stricmp(ext.c_str(), ".pex")) {
    caprica::pex::PexReader rdr(script.sourceFileName.string());
    auto pex = caprica::pex::PexFile::read(rdr);
    reportingContext.exitIfErrors();
    std::ofstream asmStrm(script.outputDirectory + "\\" + baseName + ".pas", std::ofstream::binary);
    caprica::pex::PexAsmWriter asmWtr(asmStrm);
    pex->writeAsm(asmWtr);
    delete pex;
  } else {
    std::cout << "Don't know how to compile " << script.sourceFileName << "!" << std::endl;
  }
}

static std::pair<std::string, std::string> parseOddArguments(const std::string& str) {
  if (str == "-WE")
    return std::make_pair("all-warnings-as-errors", "");
  else if (str.find("-we") == 0)
    return std::make_pair("warning-as-error", str.substr(3));
  else if (str.find("-wd") == 0)
    return std::make_pair("disable-warning", str.substr(3));
  else
    return std::make_pair(std::string(), std::string());
}

static bool parseArgs(int argc, char* argv[], std::vector<ScriptToCompile>& filesToCompile) {
  namespace po = boost::program_options;

  try {
    bool iterateCompiledDirectoriesRecursively = false;

    po::options_description desc("General");
    desc.add_options()
      ("help,h", "Print usage information.")
      ("import,i", po::value<std::vector<std::string>>()->composing(), "Set the compiler's import directories.")
      ("flags,f", po::value<std::string>(), "Set the file defining the user flags.")
      ("optimize,O", po::bool_switch(&conf::CodeGeneration::enableOptimizations)->default_value(false), "Enable optimizations.")
      ("output,o", po::value<std::string>()->default_value(boost::filesystem::current_path().string()), "Set the directory to save compiler output to.")
      ("parallel-compile,p", po::bool_switch(&conf::General::compileInParallel)->default_value(false), "Compile files in parallel.")
      ("recurse,r", po::bool_switch(&iterateCompiledDirectoriesRecursively)->default_value(false), "Recursively compile all scripts in the directories passed.")
      ("all-warnings-as-errors", po::bool_switch(&conf::Warnings::treatWarningsAsErrors)->default_value(false), "Treat all warnings as if they were errors.")
      ("disable-all-warnings", po::bool_switch(&conf::Warnings::disableAllWarnings)->default_value(false), "Disable all warnings by default.")
      ("warning-as-error", po::value<std::vector<size_t>>()->composing(), "Treat a specific warning as an error.")
      ("disable-warning", po::value<std::vector<size_t>>()->composing(), "Disable a specific warning.")
      ("config-file", po::value<std::string>()->default_value("caprica.cfg"), "Load additional options from a config file.")
      ("quiet,q", po::bool_switch(&conf::General::quietCompile)->default_value(false), "Do not report progress, only failures.")
    ;

    po::options_description champollionCompatDesc("Champollion Compatibility");
    champollionCompatDesc.add_options()
      ("champollion-compat", po::value<bool>()->default_value(true)->implicit_value(true), "Enable a few options that make it easier to compile Papyrus code decompiled by Champollion.")
      ("allow-compiler-identifiers", po::bool_switch(&conf::Papyrus::allowCompilerIdentifiers)->default_value(false), "Allow identifiers to be prefixed with ::, which is normally reserved for compiler identifiers.")
      ("allow-decompiled-struct-references", po::bool_switch(&conf::Papyrus::allowDecompiledStructNameRefs)->default_value(false), "Allow the name of structs to be prefixed with the containing object name, followed by a #, then the name of the struct.")
    ;

    po::options_description advancedDesc("Advanced");
    advancedDesc.add_options()
      ("allow-negative-literal-as-binary-op", po::value<bool>(&conf::Papyrus::allowNegativeLiteralAsBinaryOp)->default_value(true), "Allow a negative literal number to be parsed as a binary op.")
      ("async-read", po::value<bool>(&conf::Performance::asyncFileRead)->default_value(true), "Allow async file reading. This is primarily useful on SSDs.")
      ("async-write", po::value<bool>(&conf::Performance::asyncFileWrite)->default_value(true), "Allow writing output to disk on background threads.")
      ("dump-asm", po::bool_switch(&conf::Debug::dumpPexAsm)->default_value(false), "Dump the PEX assembly code for the input files.")
      ("enable-ck-optimizations", po::value<bool>(&conf::CodeGeneration::enableCKOptimizations)->default_value(true), "Enable optimizations that the CK compiler normally does regardless of the -optimize switch.")
      ("enable-debug-info", po::value<bool>(&conf::CodeGeneration::emitDebugInfo)->default_value(true), "Enable the generation of debug info. Disabling this will result in Property Groups not showing up in the Creation Kit for the compiled script. This also removes the line number and struct order information.")
      ("enable-language-extensions", po::value<bool>(&conf::Papyrus::enableLanguageExtensions)->default_value(true), "Enable Caprica's extensions to the Papyrus language.")
      ("resolve-symlinks", po::value<bool>(&conf::Performance::resolveSymlinks)->default_value(false), "Fully resolve symlinks when determining file paths.")
    ;

    po::options_description hiddenDesc("");
    hiddenDesc.add_options()
      ("input-file", po::value<std::vector<std::string>>(), "The input file.")

      // These are intended for debugging, not general use.
      ("debug-control-flow-graph", po::value<bool>(&conf::Debug::debugControlFlowGraph)->default_value(false), "Dump the control flow graph for every function to std::cout.")
      ("performance-test-mode", po::bool_switch(&conf::Performance::performanceTestMode)->default_value(false), "Enable performance test mode.")
    ;

    po::options_description engineLimitsDesc("");
    engineLimitsDesc.add_options()
      ("ignore-engine-limits", po::bool_switch(&conf::EngineLimits::ignoreLimits)->default_value(false), "Warn when breaking a game engine limitation, but allow the compile to continue anyways.")
      ("engine-limits-max-array-length", po::value<size_t>(&conf::EngineLimits::maxArrayLength)->default_value(128), "The maximum length of an array. 0 means no limit.")
      ("engine-limits-max-functions-in-empty-state-per-object", po::value<size_t>(&conf::EngineLimits::maxFunctionsInEmptyStatePerObject)->default_value(2047), "The maximum number of functions in the empty state in a single object. 0 means no limit.")
      ("engine-limits-max-functions-per-state", po::value<size_t>(&conf::EngineLimits::maxFunctionsPerState)->default_value(511), "The maximum number of functions in a single state. 0 means no limit.")
      ("engine-limits-max-initial-values-per-object", po::value<size_t>(&conf::EngineLimits::maxInitialValuesPerObject)->default_value(1023), "The maximum number of variables in a single object that can have initial values. 0 means no limit.")
      ("engine-limits-max-named-states-per-object", po::value<size_t>(&conf::EngineLimits::maxNamedStatesPerObject)->default_value(127), "The maximum number of named states in a single object. 0 means no limit.")
      ("engine-limits-max-parameters-per-function", po::value<size_t>(&conf::EngineLimits::maxParametersPerFunction)->default_value(511), "The maximum number of parameters to a single function. 0 means no limit.")
      ("engine-limits-max-properties-per-object", po::value<size_t>(&conf::EngineLimits::maxPropertiesPerObject)->default_value(1023), "The maximum number of properties in a single object. 0 means no limit.")
      ("engine-limits-max-static-functions-per-object", po::value<size_t>(&conf::EngineLimits::maxStaticFunctionsPerObject)->default_value(511), "The maximum number of global functions allowed in a single object. 0 means no limit.")
      ("engine-limits-max-user-flags", po::value<size_t>(&conf::EngineLimits::maxUserFlags)->default_value(31), "The maximum number of distinct user flags allowed. Composite flags do not count toward this limit.")
      ("engine-limits-max-variables-per-object", po::value<size_t>(&conf::EngineLimits::maxVariablesPerObject)->default_value(1023), "The maximum number of variables in a single object. 0 means no limit.")
    ;

    po::positional_options_description p;
    p.add("input-file", -1);

    po::options_description visibleDesc("");
    visibleDesc.add(desc).add(champollionCompatDesc).add(advancedDesc);

    po::options_description commandLineDesc("");
    commandLineDesc.add(visibleDesc).add(hiddenDesc).add(engineLimitsDesc);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
              .options(commandLineDesc)
              .extra_parser(parseOddArguments)
              .positional(p)
              .run(), vm);
    po::notify(vm);

    auto confFilePath = vm["config-file"].as<std::string>();
    auto progamBasePath = boost::filesystem::absolute(boost::filesystem::path(argv[0]).parent_path()).string();
    bool loadedConfigFile = false;
    if (boost::filesystem::exists(progamBasePath + "\\" + confFilePath)) {
      loadedConfigFile = true;
      std::ifstream ifs(progamBasePath + "\\" + confFilePath);
      po::store(po::parse_config_file(ifs, commandLineDesc), vm);
      po::notify(vm);
    }
    if (boost::filesystem::exists(confFilePath) && _stricmp(boost::filesystem::current_path().string().c_str(), progamBasePath.c_str())) {
      loadedConfigFile = true;
      std::ifstream ifs(progamBasePath + "\\" + confFilePath);
      po::store(po::parse_config_file(ifs, commandLineDesc), vm);
      po::notify(vm);
    }
    if (!loadedConfigFile && confFilePath != "caprica.cfg") {
      std::cout << "Unable to locate config file '" << confFilePath << "'." << std::endl;
      return false;
    }

    if (vm.count("help") || !vm.count("input-file")) {
      std::cout << "Caprica Papyrus Compiler v0.1.5" << std::endl;
      std::cout << "Usage: Caprica <sourceFile / directory>" << std::endl;
      std::cout << "Note that when passing a directory, only Papyrus script files (*.psc) in it will be compiled. Pex (*.pex) and Pex assembly (*.pas) files will be ignored." << std::endl;
      std::cout << visibleDesc << std::endl;
      return false;
    }

    if (vm["champollion-compat"].as<bool>()) {
      conf::Papyrus::allowCompilerIdentifiers = true;
      conf::Papyrus::allowDecompiledStructNameRefs = true;
    }

    if (vm["performance-test-mode"].as<bool>()) {
      conf::Performance::asyncFileRead = true;
      conf::Performance::asyncFileWrite = false;
    }

    if (vm.count("warning-as-error")) {
      auto warnsAsErrs = vm["warning-as-error"].as<std::vector<size_t>>();
      conf::Warnings::warningsToHandleAsErrors.reserve(warnsAsErrs.size());
      for (auto f : warnsAsErrs) {
        if (conf::Warnings::warningsToHandleAsErrors.count(f)) {
          std::cout << "Warning " << f << " was already marked as an error." << std::endl;
          return false;
        }
        conf::Warnings::warningsToHandleAsErrors.insert(f);
      }
    }

    if (vm.count("disable-warning")) {
      auto warnsIgnored = vm["disable-warning"].as<std::vector<size_t>>();
      conf::Warnings::warningsToIgnore.reserve(warnsIgnored.size());
      for (auto f : warnsIgnored) {
        if (conf::Warnings::warningsToIgnore.count(f)) {
          std::cout << "Warning " << f << " was already disabled." << std::endl;
          return false;
        }
        conf::Warnings::warningsToIgnore.insert(f);
      }
    }

    if (vm.count("import")) {
      auto dirs = vm["import"].as<std::vector<std::string>>();
      conf::Papyrus::importDirectories.reserve(dirs.size());
      for (auto d : dirs) {
        if (!boost::filesystem::exists(d)) {
          std::cout << "Unable to find the import directory '" << d << "'!" << std::endl;
          return false;
        }
        conf::Papyrus::importDirectories.push_back(caprica::FSUtils::canonical(d).string());
      }
    }

    auto baseOutputDir = vm["output"].as<std::string>();
    if (!boost::filesystem::exists(baseOutputDir))
      boost::filesystem::create_directories(baseOutputDir);
    baseOutputDir = caprica::FSUtils::canonical(baseOutputDir).string();

    if (vm.count("flags")) {
      const auto findFlags = [progamBasePath, baseOutputDir](const std::string& flagsPath) -> std::string {
        if (boost::filesystem::exists(flagsPath))
          return flagsPath;

        for (auto& i : conf::Papyrus::importDirectories) {
          if (boost::filesystem::exists(i + "\\" + flagsPath))
            return i + "\\" + flagsPath;
        }

        if (boost::filesystem::exists(baseOutputDir + "\\" + flagsPath))
          return baseOutputDir + "\\" + flagsPath;
        if (boost::filesystem::exists(progamBasePath + "\\" + flagsPath))
          return progamBasePath + "\\" + flagsPath;

        return "";
      };
      
      auto flagsPath = findFlags(vm["flags"].as<std::string>());
      if (flagsPath == "") {
        std::cout << "Unable to locate flags file '" << vm["flags"].as<std::string>() << "'." << std::endl;
        return false;
      }

      caprica::CapricaReportingContext reportingContext{ flagsPath };
      auto parser = new caprica::parser::CapricaUserFlagsParser(reportingContext, flagsPath);
      parser->parseUserFlags(conf::Papyrus::userFlagsDefinition);
      delete parser;
    }


    auto filesPassed = vm["input-file"].as<std::vector<std::string>>();
    filesToCompile.reserve(filesPassed.size());
    for (auto f : filesPassed) {
      if (!caprica::FSUtils::exists(f)) {
        std::cout << "Unable to locate input file '" << f << "'." << std::endl;
        return false;
      }
      if (boost::filesystem::is_directory(f)) {
        if (iterateCompiledDirectoriesRecursively) {
          auto absBaseDir = caprica::FSUtils::canonical(f);
          boost::system::error_code ec;
          for (const auto& e : boost::filesystem::recursive_directory_iterator(f, ec)) {
            const auto& ep = e.path();
            auto abs = caprica::FSUtils::canonical(ep);
            caprica::FSUtils::pushKnownInDirectory(abs);
            if (!wcscmp(e.path().extension().c_str(), L".psc")) {
              auto rel = caprica::FSUtils::naive_uncomplete(abs, absBaseDir).make_preferred();
              
              if (rel != ep.filename())
                filesToCompile.push_back(ScriptToCompile(ep.string(), baseOutputDir, std::move(abs), rel.parent_path()));
              else
                filesToCompile.push_back(ScriptToCompile(ep.string(), baseOutputDir, std::move(abs)));
            }
          }
        } else {
          boost::system::error_code ec;
          for (auto e : boost::filesystem::directory_iterator(f, ec)) {
            caprica::FSUtils::pushKnownInDirectory(caprica::FSUtils::canonical(e.path()));
            if (e.path().extension().string() == ".psc")
              filesToCompile.push_back(ScriptToCompile(e.path().string(), baseOutputDir, caprica::FSUtils::canonical(e.path())));
          }
        }
      } else {
        auto ext = boost::filesystem::extension(f);
        if (_stricmp(ext.c_str(), ".psc") && _stricmp(ext.c_str(), ".pas") && _stricmp(ext.c_str(), ".pex")) {
          std::cout << "Don't know how to handle input file '" << f << "'!" << std::endl;
          std::cout << "Expected either a Papyrus file (*.psc), Pex assembly file (*.pas), or a Pex file (*.pex)!" << std::endl;
          return false;
        }
        filesToCompile.push_back(ScriptToCompile(std::move(f), baseOutputDir, caprica::FSUtils::canonical(f).string()));
      }
    }
  } catch (const std::exception& ex) {
    if (ex.what() != "")
      std::cout << ex.what() << std::endl;
    return false;
  }

  return true;
}

int main(int argc, char* argv[])
{
  auto startParse = std::chrono::high_resolution_clock::now();
  std::vector<ScriptToCompile> filesToCompile;
  if (!parseArgs(argc, argv, filesToCompile)) {
    caprica::CapricaReportingContext::breakIfDebugging();
    return -1;
  }
  auto endParse = std::chrono::high_resolution_clock::now();

  auto startRead = std::chrono::high_resolution_clock::now();
  if (conf::Performance::performanceTestMode) {
    caprica::FSUtils::Cache::waitForAll();
  }
  auto endRead = std::chrono::high_resolution_clock::now();

  try {
    auto startCompile = std::chrono::high_resolution_clock::now();
    if (conf::General::compileInParallel) {
      concurrency::parallel_for_each(filesToCompile.begin(), filesToCompile.end(), [](const ScriptToCompile& fl) {
        compileScript(fl);
      });
    } else {
      for (auto& file : filesToCompile)
        compileScript(file);
    }
    auto endCompile = std::chrono::high_resolution_clock::now();
    if (conf::Performance::performanceTestMode) {
      std::cout << "Parse: " << std::chrono::duration_cast<std::chrono::milliseconds>(endParse - startParse).count() << "ms" << std::endl;
      std::cout << "Read: "  << std::chrono::duration_cast<std::chrono::milliseconds>(endRead - startRead).count() << "ms" << std::endl;
      std::cout << "Compiled " << filesToCompile.size() << " files in " << std::chrono::duration_cast<std::chrono::milliseconds>(endCompile - startCompile).count() << "ms" << std::endl;
      //getc(stdin);
    }
  } catch (const std::runtime_error& ex) {
    if (ex.what() != "")
      std::cout << ex.what() << std::endl;
    caprica::CapricaReportingContext::breakIfDebugging();
    return -1;
  }

  return 0;
}

