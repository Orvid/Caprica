
#include <boost/program_options.hpp>

#include <common/CapricaConfig.h>
#include <common/FSUtils.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <papyrus/PapyrusCompilationContext.h>

namespace conf = caprica::conf;
namespace po = boost::program_options;
namespace filesystem = std::filesystem;

namespace caprica {
struct CapricaJobManager;

bool addFilesFromDirectory(const std::string &f, bool recursive, const std::string &baseOutputDir,
                           caprica::CapricaJobManager *jobManager,
                           caprica::papyrus::PapyrusCompilationNode::NodeType nodeType);
void parseUserFlags(std::string&& flagsPath);
bool handleImports(const std::vector<std::string>& f, caprica::CapricaJobManager* jobManager);
bool addSingleFile(const std::string &f, const std::string &baseOutputDir, caprica::CapricaJobManager * jobManager, caprica::papyrus::PapyrusCompilationNode::NodeType nodeType);

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

bool parseCommandLineArguments(int argc, char* argv[], caprica::CapricaJobManager* jobManager) {
  try {
    bool iterateCompiledDirectoriesRecursively = false;

    po::options_description desc("General");
    desc.add_options()
      ("help,h", "Print usage information.")
      ("game,g", po::value<std::string>()->default_value("starfield"), "Set the game type to compile for. Valid values are: starfield, skyrim, fallout4, fallout76. (default: starfield)")
      ("import,i", po::value<std::vector<std::string>>()->composing(), "Set the compiler's import directories.")
      ("flags,f", po::value<std::string>(), "Set the file defining the user flags.")
      ("optimize,O", po::bool_switch(&conf::CodeGeneration::enableOptimizations)->default_value(false), "Enable optimizations.")
      ("output,o", po::value<std::string>()->default_value(filesystem::current_path().string()), "Set the directory to save compiler output to.")
      ("parallel-compile,p", po::bool_switch(&conf::General::compileInParallel)->default_value(false), "Compile files in parallel.")
      ("recurse,r", po::bool_switch(&iterateCompiledDirectoriesRecursively)->default_value(false), "Recursively compile all scripts in the directories passed.")
      ("release", po::bool_switch(&conf::CodeGeneration::disableDebugCode)->default_value(false), "Don't generate DebugOnly code.")
      ("final", po::bool_switch(&conf::CodeGeneration::disableBetaCode)->default_value(false), "Don't generate BetaOnly code.")
      ("all-warnings-as-errors", po::bool_switch(&conf::Warnings::treatWarningsAsErrors)->default_value(false), "Treat all warnings as if they were errors.")
      ("disable-all-warnings", po::bool_switch(&conf::Warnings::disableAllWarnings)->default_value(false), "Disable all warnings by default.")
      ("warning-as-error", po::value<std::vector<size_t>>()->composing(), "Treat a specific warning as an error.")
      ("disable-warning", po::value<std::vector<size_t>>()->composing(), "Disable a specific warning.")
      ("config-file", po::value<std::string>()->default_value("caprica.cfg"), "Load additional options from a config file.")
      ("quiet,q", po::bool_switch(&conf::General::quietCompile)->default_value(false), "Do not report progress, only failures.")
      ("strict", po::value<bool>()->default_value(false)->implicit_value(true), "Enable strict checking of control flow, poisoning, and more sane implicit conversions. It is strongly recommended to enable these.")
      ;

    po::options_description champollionCompatDesc("Champollion Compatibility");
    champollionCompatDesc.add_options()
      ("champollion-compat", po::value<bool>()->default_value(true)->implicit_value(true), "Enable a few options that make it easier to compile Papyrus code decompiled by Champollion.")
      ("allow-compiler-identifiers", po::bool_switch(&conf::Papyrus::allowCompilerIdentifiers)->default_value(false), "Allow identifiers to be prefixed with ::, which is normally reserved for compiler identifiers.")
      ("allow-decompiled-struct-references", po::bool_switch(&conf::Papyrus::allowDecompiledStructNameRefs)->default_value(false), "Allow the name of structs to be prefixed with the containing object name, followed by a #, then the name of the struct.")
      ;

    po::options_description strictChecksDesc("Strict Compilation Checks");
    strictChecksDesc.add_options()
      ("require-all-paths-return", po::bool_switch()->default_value(false), "Require all control paths to return a value")
      ("ensure-betaonly-debugonly-dont-escape", po::bool_switch()->default_value(false), "Ensure values returned from BetaOnly and DebugOnly functions don't escape, as that will cause invalid code generation.")
      ("disable-implicit-conversion-from-none", po::bool_switch()->default_value(false), "Disable implicit conversion from None in most situations where the use of None likely wasn't the author's intention.")
            ("skyrim-allow-unknown-events-on-non-native-class", po::value<bool>(&conf::Skyrim::skyrimAllowUnknownEventsOnNonNativeClass)->default_value(false), "Allow unknown events to be defined on non-native classes. This is encountered with some scripts in the base game having Events that are not present on ObjectReference.")

            ;

    po::options_description skyrimCompatibilityDesc("Skyrim compatibility (default true with '--game=skyrim')");
    skyrimCompatibilityDesc.add_options()
      ("allow-unknown-events", po::value<bool>(&conf::Skyrim::skyrimAllowUnknownEventsOnNonNativeClass)->default_value(true), "Allow unknown events to be defined on non-native classes. This is encountered with some scripts in the base game having Events that are not present on ObjectReference.")
      ("allow-var-shadow-parent", po::value<bool>(&conf::Skyrim::skyrimAllowObjectVariableShadowingParentProperty)->default_value(true), "Allow Object variable names in derived classes to shadow properties in parent classes.")
      ("allow-local-use-before-decl", po::value<bool>(&conf::Skyrim::skyrimAllowLocalUseBeforeDeclaration)->default_value(true), "Allow local variables to be used before they are declared and initialized.")
      ("allow-assign-void-method-result", po::value<bool>(&conf::Skyrim::skyrimAllowAssigningVoidMethodCallResult)->default_value(true), "Allow void method call results to be assigned to Objects and Bools.")
      ;

    po::options_description advancedDesc("Advanced");
    advancedDesc.add_options()
      ("allow-negative-literal-as-binary-op", po::value<bool>(&conf::Papyrus::allowNegativeLiteralAsBinaryOp)->default_value(true), "Allow a negative literal number to be parsed as a binary op.")
      ("async-read", po::value<bool>(&conf::Performance::asyncFileRead)->default_value(true), "Allow async file reading. This is primarily useful on SSDs.")
      ("async-write", po::value<bool>(&conf::Performance::asyncFileWrite)->default_value(true), "Allow writing output to disk on background threads.")
      ("dump-asm", po::bool_switch(&conf::Debug::dumpPexAsm)->default_value(false), "Dump the PEX assembly code for the input files.")
      ("enable-ck-optimizations", po::value<bool>(&conf::CodeGeneration::enableCKOptimizations)->default_value(true), "Enable optimizations that the CK compiler normally does regardless of the -optimize switch.")
      ("enable-debug-info", po::value<bool>(&conf::CodeGeneration::emitDebugInfo)->default_value(true), "Enable the generation of debug info. Disabling this will result in Property Groups not showing up in the Creation Kit for the compiled script. This also removes the line number and struct order information.")
      ("enable-language-extensions", po::value<bool>(&conf::Papyrus::enableLanguageExtensions)->default_value(false), "Enable Caprica's extensions to the Papyrus language.")
      ("resolve-symlinks", po::value<bool>(&conf::Performance::resolveSymlinks)->default_value(false), "Fully resolve symlinks when determining file paths.")
      ;

    po::options_description hiddenDesc("");
    hiddenDesc.add_options()
      ("input-file", po::value<std::vector<std::string>>(), "The input file.")

      // These are intended for debugging, not general use.
      ("force-enable-optimizations", po::bool_switch()->default_value(false), "Force optimizations to be enabled.")
      ("debug-control-flow-graph", po::value<bool>(&conf::Debug::debugControlFlowGraph)->default_value(false), "Dump the control flow graph for every function to std::cout.")
      ("performance-test-mode", po::bool_switch(&conf::Performance::performanceTestMode)->default_value(false), "Enable performance test mode.")
      ("dump-timing", po::bool_switch(&conf::Performance::dumpTiming)->default_value(false), "Dump timing info.")
      ;

    po::options_description engineLimitsDesc("");
    engineLimitsDesc.add_options()
      ("ignore-engine-limits", po::bool_switch(&conf::EngineLimits::ignoreLimits)->default_value(false), "Warn when breaking a game engine limitation, but allow the compile to continue anyways.")
      ("engine-limits-max-array-length", po::value<size_t>(&conf::EngineLimits::maxArrayLength)->default_value(128), "The maximum length of an array. 0 means no limit.")
      ("engine-limits-max-functions-in-empty-state-per-object", po::value<size_t>(&conf::EngineLimits::maxFunctionsInEmptyStatePerObject)->default_value(2047), "The maximum number of functions in the empty state in a single object. 0 means no limit.")
      ("engine-limits-max-functions-per-state", po::value<size_t>(&conf::EngineLimits::maxFunctionsPerState)->default_value(511), "The maximum number of functions in a single state. 0 means no limit.")
      ("engine-limits-max-guards-per-object", po::value<size_t>(&conf::EngineLimits::maxGuardsPerObject)->default_value(511), "The maximum number of guards in a single object. 0 means no limit.") // TODO: Verify Starfield Guard object limits
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
    visibleDesc.add(desc).add(champollionCompatDesc).add(skyrimCompatibilityDesc).add(advancedDesc);

    po::options_description commandLineDesc("");
    commandLineDesc.add(visibleDesc).add(hiddenDesc).add(engineLimitsDesc).add(strictChecksDesc);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
              .options(commandLineDesc)
              .extra_parser(parseOddArguments)
              .positional(p)
              .run(), vm);
    po::notify(vm);

    std::string confFilePath = vm["config-file"].as<std::string>();
    auto progamBasePath = filesystem::absolute(filesystem::path(argv[0]).parent_path()).string();
    bool loadedConfigFile = false;
    if (filesystem::exists(progamBasePath + "\\" + confFilePath)) {
      loadedConfigFile = true;
      std::ifstream ifs(progamBasePath + "\\" + confFilePath);
      po::store(po::parse_config_file(ifs, commandLineDesc), vm);
      po::notify(vm);
    }
    if (filesystem::exists(confFilePath) && _stricmp(filesystem::current_path().string().c_str(), progamBasePath.c_str())) {
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
      std::cout << "Caprica Papyrus Compiler v0.2.0" << std::endl;
      std::cout << "Usage: Caprica <sourceFile / directory>" << std::endl;
      std::cout << "Note that when passing a directory, only Papyrus script files (*.psc) in it will be compiled. Pex (*.pex) and Pex assembly (*.pas) files will be ignored." << std::endl;
      std::cout << visibleDesc << std::endl;
      return false;
    }

    std::string gameType = vm["game"].as<std::string>();
    if (_stricmp(gameType.c_str(), "Starfield") == 0){
      conf::Papyrus::game = GameID::Starfield;
    } else if (_stricmp(gameType.c_str(), "Skyrim") == 0) {
      conf::Papyrus::game = GameID::Skyrim;
    } else if (_stricmp(gameType.c_str(), "Fallout4") == 0) {
      conf::Papyrus::game = GameID::Fallout4;
    } else if (_stricmp(gameType.c_str(), "Fallout76") == 0) {
      conf::Papyrus::game = GameID::Fallout76;
    } else {
      std::cout << "Unrecognized game type '" << gameType << "'!" << std::endl;
      return false;
    }

    if (conf::Papyrus::game != GameID::Skyrim){
      // turn off skyrim options
      conf::Skyrim::skyrimAllowUnknownEventsOnNonNativeClass = false;
      conf::Skyrim::skyrimAllowObjectVariableShadowingParentProperty = false;
      conf::Skyrim::skyrimAllowLocalUseBeforeDeclaration = false;
      conf::Skyrim::skyrimAllowAssigningVoidMethodCallResult = false;
    }


    // TODO: enable this eventually
    if (vm["optimize"].as<bool>() && conf::Papyrus::game != GameID::Fallout4) {
      if (!vm["force-enable-optimizations"].as<bool>()){
        conf::CodeGeneration::enableOptimizations = false;
        std::cout << "Warning: Optimization is currently only supported for Fallout 4, disabling..." << std::endl;
      } else {
        std::cout << "Warning: Optimization force enabled, optimization is currently only supported for Fallout 4 and may produce incorrect code." << std::endl;
      }
    }

    if (vm["champollion-compat"].as<bool>()) {
      conf::Papyrus::allowCompilerIdentifiers = true;
      conf::Papyrus::allowDecompiledStructNameRefs = true;
    }

    if (vm["performance-test-mode"].as<bool>()) {
      conf::Performance::dumpTiming = true;
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

    if (vm["strict"].as<bool>()) {
      for (size_t i = 1000; i < 1010; i++) {
        conf::Warnings::warningsToHandleAsErrors.insert(i);
      }
    } else if (vm["require-all-paths-return"].as<bool>()) {
      conf::Warnings::warningsToHandleAsErrors.insert(1000);
    } else if (vm["ensure-betaonly-debugonly-dont-escape"].as<bool>()) {
      conf::Warnings::warningsToHandleAsErrors.insert(1001);
      conf::Warnings::warningsToHandleAsErrors.insert(1002);
    } else if (vm["disable-implicit-conversion-from-none"].as<bool>()) {
      conf::Warnings::warningsToHandleAsErrors.insert(1003);
    }

    if (vm.count("disable-warning")) {
      std::vector<size_t> warnsIgnored = vm["disable-warning"].as<std::vector<size_t>>();
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
      for (auto& d : dirs) {
        if (!filesystem::exists(d)) {
          std::cout << "Unable to find the import directory '" << d << "'!" << std::endl;
          return false;
        }
        conf::Papyrus::importDirectories.push_back(caprica::FSUtils::canonical(d));
      }
    }

    std::string baseOutputDir = vm["output"].as<std::string>();
    if (!filesystem::exists(baseOutputDir))
      filesystem::create_directories(baseOutputDir);
    baseOutputDir = FSUtils::canonical(baseOutputDir);

    if (vm.count("flags")) {
      const auto findFlags = [progamBasePath, baseOutputDir](const std::string& flagsPath) -> std::string {
        if (filesystem::exists(flagsPath))
          return flagsPath;

        for (auto& i : conf::Papyrus::importDirectories) {
          if (filesystem::exists(i + "\\" + flagsPath))
            return i + "\\" + flagsPath;
        }

        if (filesystem::exists(baseOutputDir + "\\" + flagsPath))
          return baseOutputDir + "\\" + flagsPath;
        if (filesystem::exists(progamBasePath + "\\" + flagsPath))
          return progamBasePath + "\\" + flagsPath;

        return "";
      };

      auto flagsPath = findFlags(vm["flags"].as<std::string>());
      if (flagsPath == "") {
        std::cout << "Unable to locate flags file '" << vm["flags"].as<std::string>() << "'." << std::endl;
        return false;
      }

      parseUserFlags(std::move(flagsPath));
    }


    if (!handleImports(conf::Papyrus::importDirectories, jobManager)){
      std::cout << "Import failed!" << std::endl;
      return false;
    }

    auto filesPassed = vm["input-file"].as<std::vector<std::string>>();
    for (auto& f : filesPassed) {
      if (!filesystem::exists(f)) {
        std::cout << "Unable to locate input file '" << f << "'." << std::endl;
        return false;
      }
      if (filesystem::is_directory(f)) {
        if (!addFilesFromDirectory(f, iterateCompiledDirectoriesRecursively, baseOutputDir, jobManager,
                                   caprica::papyrus::PapyrusCompilationNode::NodeType::PapyrusCompile))
          return false;
      } else {
        std::cout << "WARNING: Loose input files are assumed as being in the root namespace." << std::endl;
        std::string_view ext = FSUtils::extensionAsRef(f);
        if (!pathEq(ext, ".psc") && !pathEq(ext, ".pas") && !pathEq(ext, ".pex")) {
          std::cout << "Don't know how to handle input file '" << f << "'!" << std::endl;
          std::cout << "Expected either a Papyrus file (*.psc), Pex assembly file (*.pas), or a Pex file (*.pex)!" << std::endl;
          return false;
        }
        auto oDir = baseOutputDir;
        addSingleFile(std::move(f), std::move(oDir), jobManager, caprica::papyrus::PapyrusCompilationNode::NodeType::PapyrusCompile);
      }
    }
  } catch (const std::exception& ex) {
    if (ex.what() != std::string(""))
      std::cout << ex.what() << std::endl;
    return false;
  }

  return true;
}


}