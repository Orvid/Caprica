
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <common/CapricaConfig.h>
#include <common/FSUtils.h>
#include <common/parser/CapricaPPJParser.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <papyrus/PapyrusCompilationContext.h>
#include <string>
#include <utility>
namespace conf = caprica::conf;
namespace po = boost::program_options;
namespace filesystem = std::filesystem;

namespace caprica {
struct CapricaJobManager;

bool addFilesFromDirectory(const IInputFile& input,
                           const std::filesystem::path& baseOutputDir,
                           caprica::CapricaJobManager* jobManager,
                           papyrus::PapyrusCompilationNode::NodeType nodeType,
                           const std::string& startingNS = "");
void parseUserFlags(std::string&& flagsPath);
bool handleImports(const std::vector<ImportDir>& f, caprica::CapricaJobManager* jobManager);
bool addSingleFile(const IInputFile& input,
                   const std::filesystem::path& baseOutputDir,
                   caprica::CapricaJobManager* jobManager,
                   caprica::papyrus::PapyrusCompilationNode::NodeType nodeType);

static std::pair<std::string, std::string> parseOddArguments(const std::string& str) {
  // Boost refuses to allow short options to have equals signs, so we have to parse them manually.
  if (str.starts_with("-i="))
    return std::make_pair("import", str.substr(3));
  else if (str.starts_with("-o="))
    return std::make_pair("output", str.substr(3));
  else if (str.starts_with("-f="))
    return std::make_pair("flags", str.substr(3));
  else if (str.starts_with("-g="))
    return std::make_pair("game", str.substr(3));
  if (str == "-WE")
    return std::make_pair("all-warnings-as-errors", "");
  else if (str.find("-we") == 0)
    return std::make_pair("warning-as-error", str.substr(3));
  else if (str.find("-wd") == 0)
    return std::make_pair("disable-warning", str.substr(3));
  else
    return std::make_pair(std::string(), std::string());
}
void SaveDefaultConfig(const po::options_description& descOptions,
                       const std::string& configFilePath_,
                       const boost::program_options::variables_map& vm) {
  std::ofstream configFile(configFilePath_);
  boost::property_tree::ptree tree;

  for (auto& option : descOptions.options()) {
    std::string name = option->long_name();
    // don't write this to the config file
    if (name == "write-config-file" || name == "input-file")
      continue;
    boost::any defaultValue;
    boost::any realValue = vm[name].value();
    option->semantic()->apply_default(defaultValue);
    if (realValue.empty())
      realValue = defaultValue;
    if (realValue.type() == typeid(std::string)) {
      std::string val = boost::any_cast<std::string>(realValue);
      tree.put(name, val);
      /// Add here additional else.. type() == typeid() if necessary
    } else if (realValue.type() == typeid(bool)) {
      bool val = boost::any_cast<bool>(realValue);
      tree.put(name, val);
    } else if (realValue.type() == typeid(size_t)) {
      size_t val = boost::any_cast<size_t>(realValue);
      tree.put(name, val);
    } else if (realValue.type() == typeid(int)) {
      int val = boost::any_cast<int>(realValue);
      tree.put(name, val);
    } else if (realValue.type() == typeid(double)) {
      double val = boost::any_cast<double>(realValue);
      tree.put(name, val);
    } else if (realValue.type() == typeid(std::vector<std::string>)) {
      std::vector<std::string> val = boost::any_cast<std::vector<std::string>>(realValue);
      std::string strVal;
      for (auto& v : val)
        strVal += v + ";";
      tree.put(name, strVal);
    } else if (realValue.type() == typeid(std::vector<size_t>)) {
      std::vector<size_t> val = boost::any_cast<std::vector<size_t>>(realValue);
      std::string strVal;
      for (auto& v : val)
        strVal += std::to_string(v) + ",";
      tree.put(name, strVal);
    }
  }

  // or write_ini
  boost::property_tree::write_ini(configFile, tree);
}

std::string findFlags(const std::filesystem::path& flagsPath,
                      const std::filesystem::path& progamBasePath,
                      const std::filesystem::path& baseOutputDir) {
  if (filesystem::exists(flagsPath))
    return flagsPath.string();

  for (auto& i : conf::Papyrus::importDirectories)
    if (filesystem::exists(i.resolved_absolute() / flagsPath))
      return (i.resolved_absolute() / flagsPath).string();

  if (filesystem::exists(baseOutputDir / flagsPath))
    return (baseOutputDir / flagsPath).string();
  if (filesystem::exists(progamBasePath / flagsPath))
    return (progamBasePath / flagsPath).string();

  return {};
};

void setPPJBool(BoolSetting value, bool& setting) {
  if (value != BoolSetting::NOT_SET)
    setting = value == BoolSetting::True ? true : false;
};

caseless_unordered_identifier_map<GameID> gameIDMap = {
  {"Starfield",  GameID::Starfield},
  { "Skyrim",    GameID::Skyrim   },
  { "Fallout4",  GameID::Fallout4 },
  { "Fallout76", GameID::Fallout76},
};

bool setGame(std::string gameType) {
  auto it = gameIDMap.find(gameType);
  if (it != gameIDMap.end()) {
    conf::Papyrus::game = it->second;
    return true;
  }
  return false;
}

constexpr const char* DEFAULT_GAME = "Starfield";

bool parseCommandLineArguments(int argc, char* argv[], caprica::CapricaJobManager* jobManager) {
  try {
    bool iterateCompiledDirectoriesRecursively = false;

    // clang-format off
    po::options_description desc("General");
    desc.add_options()
      ("help,h,?", "Print usage information.")
      ("game,g", po::value<std::string>(),
        "Set the game type to compile for. Valid values are: starfield, skyrim, fallout4, fallout76. (default: starfield)")
      ("import,i", po::value<std::vector<std::string>>()->composing(),
        "Set the compiler's import directories.")
      ("flags,f", po::value<std::string>(),
        "Set the file defining the user flags.")
      ("output,o", po::value<std::string>(),
        "Set the directory to save compiler output to.")
      ("optimize,op,O",
        "Enable optimizations.")
      ("parallel-compile,p", po::bool_switch(&conf::General::compileInParallel)->default_value(false),
        "Compile files in parallel.")
      ("release,r",
        "Don't generate DebugOnly code.")
      ("recurse,R", po::bool_switch(&iterateCompiledDirectoriesRecursively)->default_value(false),
        "Recursively compile all scripts in the directories passed.")
      ("final",
        "Don't generate BetaOnly code.")
      ("anonymize", "Anonymize script header information.")
      ("all-warnings-as-errors",
        po::bool_switch(&conf::Warnings::treatWarningsAsErrors)->default_value(false),
        "Treat all warnings as if they were errors.")
      ("disable-all-warnings", po::bool_switch(&conf::Warnings::disableAllWarnings)->default_value(false),
        "Disable all warnings by default.")
      ("warning-as-error", po::value<std::vector<size_t>>()->composing(),
        "Treat a specific warning as an error.")
      ("disable-warning", po::value<std::vector<size_t>>()->composing(),
        "Disable a specific warning.")
      ("config-file", po::value<std::string>()->default_value("caprica.cfg"),
        "Load additional options from a config file.")
      ("quiet,q", po::bool_switch(&conf::General::quietCompile)->default_value(false),
        "Do not report progress, only failures.")
      ("strict", po::value<bool>()->default_value(false)->implicit_value(true),
        "Enable strict checking of control flow, poisoning, and more sane implicit conversions. It is strongly recommended to enable these.")
      ("ignorecwd", po::bool_switch()->default_value(false)->implicit_value(true),
        "Do not add the current working directory to the beginning of the import list.");

    po::options_description champollionCompatDesc("Champollion Compatibility");
    champollionCompatDesc.add_options()
      ("champollion-compat", po::value<bool>()->default_value(true)->implicit_value(true),
        "Enable a few options that make it easier to compile Papyrus code decompiled by Champollion.")
      ("allow-compiler-identifiers", po::bool_switch(&conf::Papyrus::allowCompilerIdentifiers)->default_value(false),
        "Allow identifiers to be prefixed with ::, which is normally reserved for compiler identifiers.")
      ("allow-decompiled-struct-references", po::bool_switch(&conf::Papyrus::allowDecompiledStructNameRefs)->default_value(false),
        "Allow the name of structs to be prefixed with the containing object name, followed by a #, then the name of the struct.");

    po::options_description strictChecksDesc("Strict Compilation Checks");
    strictChecksDesc.add_options()
      ("require-all-paths-return", po::bool_switch()->default_value(false),
        "Require all control paths to return a value")
      ("ensure-betaonly-debugonly-dont-escape", po::bool_switch()->default_value(false),
        "Ensure values returned from BetaOnly and DebugOnly functions don't escape, as that will cause invalid code generation.")
      ("disable-implicit-conversion-from-none", po::bool_switch()->default_value(false),
        "Disable implicit conversion from None in most situations where the use of None likely wasn't the author's intention.")
      ("disable-keywords-as-identifiers", po::bool_switch()->default_value(false),
        "Disable the ability to use some keywords (e.g. Switch, Parent) as identifiers in select cases.")
      ("skyrim-allow-unknown-events-on-non-native-class", po::value<bool>(&conf::Skyrim::skyrimAllowUnknownEventsOnNonNativeClass)->default_value(true),
        "Allow unknown events to be defined on non-native classes. This is encountered with some scripts in the base game having Events that are not present on ObjectReference.");

    po::options_description skyrimCompatibilityDesc("Skyrim compatibility (default true with '--game=skyrim')");
    skyrimCompatibilityDesc.add_options()
      ("allow-unknown-events", po::value<bool>(&conf::Skyrim::skyrimAllowUnknownEventsOnNonNativeClass)->default_value(true),
        "Allow unknown events to be defined on non-native classes. This is encountered with some scripts in the base game having Events that are not present on ObjectReference.")
      ("allow-var-shadow-parent", po::value<bool>(&conf::Skyrim::skyrimAllowObjectVariableShadowingParentProperty)->default_value(true),
        "Allow Object variable names in derived classes to shadow properties in parent classes.")
      ("allow-local-shadow-parent", po::value<bool>(&conf::Skyrim::skyrimAllowLocalVariableShadowingParentProperty)->default_value(true),
        "Allow local variable names to shadow properties in parent classes.")
      ("allow-local-use-before-decl", po::value<bool>(&conf::Skyrim::skyrimAllowLocalUseBeforeDeclaration)->default_value(true),
        "Allow local variables to be used before they are declared and initialized.")
      ("allow-assign-void-method-result", po::value<bool>(&conf::Skyrim::skyrimAllowAssigningVoidMethodCallResult)->default_value(true),
        "Allow void method call results to be assigned to Objects and Bools.");

    po::options_description advancedDesc("Advanced");
    advancedDesc.add_options()
      ("allow-negative-literal-as-binary-op", po::value<bool>(&conf::Papyrus::allowNegativeLiteralAsBinaryOp)->default_value(true),
        "Allow a negative literal number to be parsed as a binary op.")
      ("async-read", po::value<bool>(&conf::Performance::asyncFileRead)->default_value(true),
        "Allow async file reading. This is primarily useful on SSDs.")
      ("async-write", po::value<bool>(&conf::Performance::asyncFileWrite)->default_value(true),
        "Allow writing output to disk on background threads.")
      ("dump-asm,keepasm",
        "Dump the PEX assembly code for the input files.")
      ("enable-ck-optimizations", po::value<bool>(&conf::CodeGeneration::enableCKOptimizations)->default_value(true),
        "Enable optimizations that the CK compiler normally does regardless of the -optimize switch.")
      ("enable-debug-info", po::value<bool>(&conf::CodeGeneration::emitDebugInfo)->default_value(true),
        "Enable the generation of debug info. Disabling this will result in Property Groups not showing up in the "
        "Creation Kit for the compiled script. This also removes the line number and struct order information.")
      ("enable-language-extensions", po::value<bool>(&conf::Papyrus::enableLanguageExtensions)->default_value(false),
        "Enable Caprica's extensions to the Papyrus language.")
      ("resolve-symlinks", po::value<bool>(&conf::Performance::resolveSymlinks)->default_value(false),
        "Fully resolve symlinks when determining file paths.");

    po::options_description hiddenDesc("");
    hiddenDesc.add_options()
      ("input-file", po::value<std::vector<std::string>>(), "The input file.")
      // These are intended for debugging, not general use.
      ("force-enable-optimizations", po::bool_switch()->default_value(false),
        "Force optimizations to be enabled.")
      ("debug-control-flow-graph", po::value<bool>(&conf::Debug::debugControlFlowGraph)->default_value(false),
        "Dump the control flow graph for every function to std::cout.")
      ("performance-test-mode", po::bool_switch(&conf::Performance::performanceTestMode)->default_value(false),
        "Enable performance test mode.")
      ("dump-timing", po::bool_switch(&conf::Performance::dumpTiming)->default_value(false),
        "Dump timing info.")
      ("write-config-file", po::value<std::string>()->default_value(""),
        "Write the config file to disk.");

    // This is really only here for compatibility with Pyro
    po::options_description pcompilerDesc("");
    pcompilerDesc.add_options()
      ("pcompiler", po::bool_switch(&conf::PCompiler::pCompilerCompatibilityMode)->default_value(false),
        "Enable PCompiler compatibility mode (default false).")
      ("all,a", po::bool_switch(&conf::PCompiler::all)->default_value(false), "Treat input objects as directories")
      ("norecurse", po::bool_switch(&conf::PCompiler::norecurse)->default_value(false), "Don't recursively scan directories with -all")
      ("noasm", "Turns off asm output if it was turned on.")
      ("asmonly", po::bool_switch()->default_value(false), "Does nothing on Caprica")
      ("debug,d", po::bool_switch()->default_value(false), "Does nothing on Caprica");

    po::options_description engineLimitsDesc("");
    engineLimitsDesc.add_options()
      ("ignore-engine-limits", po::bool_switch(&conf::EngineLimits::ignoreLimits)->default_value(false),
        "Warn when breaking a game engine limitation, but allow the compile to continue anyways.")
      ("engine-limits-max-array-length", po::value<size_t>(&conf::EngineLimits::maxArrayLength)->default_value(128),
        "The maximum length of an array. 0 means no limit.")
      ("engine-limits-max-functions-in-empty-state-per-object", po::value<size_t>(&conf::EngineLimits::maxFunctionsInEmptyStatePerObject)->default_value(2047),
        "The maximum number of functions in the empty state in a single object. 0 means no limit.")
      ("engine-limits-max-functions-per-state", po::value<size_t>(&conf::EngineLimits::maxFunctionsPerState)->default_value(511),
        "The maximum number of functions in a single state. 0 means no limit.")
      ("engine-limits-max-guards-per-object", po::value<size_t>(&conf::EngineLimits::maxGuardsPerObject)->default_value(511),
        "The maximum number of guards in a single object. 0 means no limit.") // TODO: Verify Starfield Guard object limits
      ("engine-limits-max-initial-values-per-object", po::value<size_t>(&conf::EngineLimits::maxInitialValuesPerObject)->default_value(1023),
        "The maximum number of variables in a single object that can have initial values. 0 means no limit.")
      ("engine-limits-max-named-states-per-object", po::value<size_t>(&conf::EngineLimits::maxNamedStatesPerObject)->default_value(127),
        "The maximum number of named states in a single object. 0 means no limit.")
      ("engine-limits-max-parameters-per-function", po::value<size_t>(&conf::EngineLimits::maxParametersPerFunction)->default_value(511),
        "The maximum number of parameters to a single function. 0 means no limit.")
      ("engine-limits-max-properties-per-object", po::value<size_t>(&conf::EngineLimits::maxPropertiesPerObject)->default_value(1023),
        "The maximum number of properties in a single object. 0 means no limit.")
      ("engine-limits-max-static-functions-per-object", po::value<size_t>(&conf::EngineLimits::maxStaticFunctionsPerObject)->default_value(511),
        "The maximum number of global functions allowed in a single object. 0 means no limit.")
      ("engine-limits-max-user-flags", po::value<size_t>(&conf::EngineLimits::maxUserFlags)->default_value(31),
        "The maximum number of distinct user flags allowed. Composite flags do not count toward this limit.")
      ("engine-limits-max-variables-per-object", po::value<size_t>(&conf::EngineLimits::maxVariablesPerObject)->default_value(1023),
        "The maximum number of variables in a single object. 0 means no limit.");

    // clang-format on
    po::positional_options_description p;
    p.add("input-file", -1);

    po::options_description visibleDesc("");
    visibleDesc.add(desc).add(champollionCompatDesc).add(skyrimCompatibilityDesc).add(advancedDesc);

    po::options_description commandLineDesc("");
    commandLineDesc.add(visibleDesc).add(hiddenDesc).add(engineLimitsDesc).add(strictChecksDesc).add(pcompilerDesc);
    auto default_style = po::command_line_style::unix_style;
    auto pcompiler_style = (po::command_line_style::style_t)(
        po::command_line_style::allow_short | po::command_line_style::allow_long |
        po::command_line_style::allow_dash_for_short | po::command_line_style::long_allow_adjacent |
        po::command_line_style::short_allow_adjacent | po::command_line_style::allow_long_disguise);
    bool pCompilerMode = false;
    po::variables_map vm;
    // scan argv for `-pcompiler`
    for (int i = 1; i < argc; i++) {
      if (_stricmp(argv[i], "--pcompiler") == 0 || _stricmp(argv[i], "-pcompiler") == 0) {
        default_style = pcompiler_style;
        break;
      }
    }
    po::store(po::command_line_parser(argc, argv)
                  .options(commandLineDesc)
                  .extra_parser(parseOddArguments)
                  .style(default_style)
                  .positional(p)
                  .run(),
              vm);
    po::notify(vm);
    std::string confFilePath = vm["config-file"].as<std::string>();
    auto progamBasePath = filesystem::absolute(filesystem::path(argv[0]).parent_path());
    bool loadedConfigFile = false;
    if (filesystem::exists(progamBasePath / confFilePath)) {
      loadedConfigFile = true;
      std::ifstream ifs(progamBasePath / confFilePath);
      auto config_opts = po::parse_config_file(ifs, commandLineDesc);
      po::store(config_opts, vm);
      po::notify(vm);
    }
    if (filesystem::exists(confFilePath)) {
      loadedConfigFile = true;
      auto path = std::filesystem::absolute(confFilePath);
      std::ifstream ifs { path };
      // check if ifs is open
      if (!ifs.is_open()) {
        std::cout << "Unable to open config file '" << path.string() << "'." << std::endl;
        return false;
      }
      // read the entire thing into a string
      auto config_opts = po::parse_config_file(ifs, commandLineDesc);
      po::store(config_opts, vm);
      po::notify(vm);
    }
    if (!loadedConfigFile && confFilePath != "caprica.cfg") {
      std::cout << "Unable to locate config file '" << confFilePath << "'." << std::endl;
      return false;
    }

    if (vm.count("help") || !vm.count("input-file")) {
      std::cout << "Caprica Papyrus Compiler v0.3.0" << std::endl;
      std::cout << "Usage: Caprica <sourceFile / directory>" << std::endl;
      std::cout << "Note that when passing a directory, only Papyrus script files (*.psc) in it will be compiled. Pex "
                   "(*.pex) and Pex assembly (*.pas) files will be ignored."
                << std::endl;
      std::cout << visibleDesc << std::endl;
      return false;
    }

    auto filesPassed = std::vector<std::string>();
    for (auto& f : vm["input-file"].as<std::vector<std::string>>()) {
      if (f.find(';') != std::string::npos) {
        std::istringstream s(f);
        std::string sd;
        while (getline(s, sd, ';'))
          filesPassed.push_back(sd);
      } else {
        filesPassed.push_back(f);
      }
    }

    std::string flags;
    std::filesystem::path baseOutputDir;

    PapyrusProject ppj;
    std::filesystem::path ppjPath;
    auto filesToRemove = std::vector<std::string>();
    for (auto& f : filesPassed) {
      auto ext = filesystem::path(f).extension().string();
      if (ext == ".ppj") {
        if (!ppjPath.empty()) {
          std::cout << "Only one project file can be specified!" << std::endl;
          return false;
        }
        CapricaPPJParser ppjParser;
        ppj = ppjParser.Parse(f);
        filesToRemove.push_back(f);
        ppjPath = f;
      }
    }
    // Remove the project file from the input files.
    for (auto& f : filesToRemove) {
      auto it = std::find(filesPassed.begin(), filesPassed.end(), f);
      if (it != filesPassed.end())
        filesPassed.erase(it);
    }

    if (vm.count("pcompiler")) {
      conf::PCompiler::pCompilerCompatibilityMode = vm["pcompiler"].as<bool>();
      conf::PCompiler::all = vm["all"].as<bool>();
      conf::PCompiler::norecurse = vm["norecurse"].as<bool>();
      if (conf::PCompiler::norecurse)
        iterateCompiledDirectoriesRecursively = false;
    }

    // we have to make sure that the game value is set for sure here
    // CLI flags override the project file
    if (vm.count("game")) {
      if (!ppjPath.empty() && ppj.game != PapyrusProject::PapyrusGame::UNKNOWN) {
        std::cout << "Warning: Game type specified in both project file and command line, using command line value '"
                  << vm["game"].as<std::string>() << "'." << std::endl;
      }
      if (!setGame(vm["game"].as<std::string>())) {
        std::cout << "Unrecognized game type '" << vm["game"].as<std::string>() << "'!" << std::endl;
        return false;
      }
    } else if (!ppjPath.empty() && ppj.game != PapyrusProject::PapyrusGame::UNKNOWN) {
      conf::Papyrus::game =
          ppj.game == PapyrusProject::PapyrusGame::SkyrimSpecialEdition ? GameID::Skyrim : (GameID)ppj.game;
    } else {
      setGame(DEFAULT_GAME);
    }

    if (!ppjPath.empty()) {
      auto baseDir = ppjPath.parent_path();
      if (!baseDir.empty())
        baseDir = FSUtils::canonicalFS(baseDir);
      else
        baseDir = filesystem::current_path();

      conf::Papyrus::importDirectories.reserve(conf::Papyrus::importDirectories.size() + ppj.imports.size());
      for (auto& i : ppj.imports)
        conf::Papyrus::importDirectories.emplace_back(i, false, baseDir);

      if (!vm.count("anonymize"))
        setPPJBool(ppj.anonymize, conf::General::anonymizeOutput);
      if (!vm.count("release"))
        setPPJBool(ppj.release, conf::CodeGeneration::disableDebugCode);
      if (!vm.count("final"))
        setPPJBool(ppj.finalAttr, conf::CodeGeneration::disableBetaCode);
      if (!vm.count("optimize"))
        setPPJBool(ppj.optimize, conf::CodeGeneration::enableOptimizations);
      if (!vm.count("dump-asm") && !vm.count("keep-asm") && !vm.count("noasm")) {
        switch (ppj.asmAttr) {
          case PapyrusProject::AsmType::None:
          case PapyrusProject::AsmType::Discard:
            conf::Debug::dumpPexAsm = false;
            break;
          case PapyrusProject::AsmType::Keep:
          case PapyrusProject::AsmType::Only: // TODO: HANDLE THIS
            conf::Debug::dumpPexAsm = true;
            break;
          default:
            break;
        }
      }
      auto outputDir = std::filesystem::path(ppj.output);
      if (outputDir.is_relative())
        outputDir = (baseDir / ppj.output);
      baseOutputDir = outputDir;

      conf::General::inputFiles.reserve(conf::General::inputFiles.size() + ppj.folders.size() + ppj.scripts.size());
      for (auto& f : ppj.folders){     
        conf::General::inputFiles.emplace_back(std::make_shared<PCompInputFile>(f.path, f.noRecurse, true, baseDir));
      }
      for (auto& f : ppj.scripts)
        conf::General::inputFiles.emplace_back(std::make_shared<PCompInputFile>(f, true, false, baseDir));
      flags = ppj.flags;
    } else { // if we're not doing a project file...
      // ensure cwd is placed first
      if (!vm["ignorecwd"].as<bool>()) {
        conf::Papyrus::importDirectories.reserve(conf::Papyrus::importDirectories.size() + 1);
        conf::Papyrus::importDirectories.emplace_back(filesystem::current_path(), false);
        if (!conf::General::quietCompile) {
          std::cout << "Adding current working directory to import list: "
                    << conf::Papyrus::importDirectories[0].get_unresolved_path() << std::endl;
        }
      }
    }

    if (vm.count("flags"))
      flags = vm["flags"].as<std::string>();
    if (vm.count("output"))
      baseOutputDir = vm["output"].as<std::string>();
    else if (baseOutputDir.empty())
      baseOutputDir = filesystem::current_path();

    if (vm.count("anonymize"))
      conf::General::anonymizeOutput = true;
    if (vm.count("release"))
      conf::CodeGeneration::disableDebugCode = true;
    if (vm.count("final"))
      conf::CodeGeneration::disableBetaCode = true;
    if (vm.count("optimize"))
      conf::CodeGeneration::enableOptimizations = true;
    if (vm.count("dump-asm") || vm.count("keepasm"))
      conf::Debug::dumpPexAsm = true;
    if (vm.count("pcompiler") && vm.count("noasm"))
      conf::Debug::dumpPexAsm = false;

    if (conf::Papyrus::game != GameID::Skyrim) {
      // turn off skyrim options
      conf::Skyrim::skyrimAllowUnknownEventsOnNonNativeClass = false;
      conf::Skyrim::skyrimAllowObjectVariableShadowingParentProperty = false;
      conf::Skyrim::skyrimAllowLocalVariableShadowingParentProperty = false;
      conf::Skyrim::skyrimAllowLocalUseBeforeDeclaration = false;
      conf::Skyrim::skyrimAllowAssigningVoidMethodCallResult = false;
    }

    // TODO: enable this eventually
    if (conf::CodeGeneration::enableOptimizations && conf::Papyrus::game != GameID::Fallout4) {
      if (!vm["force-enable-optimizations"].as<bool>()) {
        conf::CodeGeneration::enableOptimizations = false;
        std::cout << "Warning: Optimization is currently only supported for Fallout 4, disabling..." << std::endl;
      } else {
        std::cout << "Warning: Optimization force enabled, optimization is currently only supported for Fallout 4 and "
                     "may produce incorrect code."
                  << std::endl;
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
      for (size_t i = 1000; i < 1010; i++)
        conf::Warnings::warningsToHandleAsErrors.insert(i);
    } else if (vm["require-all-paths-return"].as<bool>()) {
      conf::Warnings::warningsToHandleAsErrors.insert(1000);
    } else if (vm["ensure-betaonly-debugonly-dont-escape"].as<bool>()) {
      conf::Warnings::warningsToHandleAsErrors.insert(1001);
      conf::Warnings::warningsToHandleAsErrors.insert(1002);
    } else if (vm["disable-implicit-conversion-from-none"].as<bool>()) {
      conf::Warnings::warningsToHandleAsErrors.insert(1003);
    } else if (vm["disable-keywords-as-identifiers"].as<bool>()) {
      conf::Warnings::warningsToHandleAsErrors.insert(1006);
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
      conf::Papyrus::importDirectories.reserve(conf::Papyrus::importDirectories.size() + dirs.size());
      for (auto& d : dirs) {
        // check if string contains `;`
        if (d.find(';') != std::string::npos) {
          std::istringstream f(d);
          std::string sd;
          while (getline(f, sd, ';')) {
            if (!filesystem::exists(sd)) {
              std::cout << "Unable to find the import directory '" << sd << "'!" << std::endl;
              return false;
            }
            conf::Papyrus::importDirectories.emplace_back(FSUtils::canonical(sd), false);
            if (!conf::General::quietCompile) {
              std::cout << "Adding import directory: " << conf::Papyrus::importDirectories.back().get_unresolved_path()
                        << std::endl;
            }
          }
          continue;
        }
        if (!filesystem::exists(d)) {
          std::cout << "Unable to find the import directory '" << d << "'!" << std::endl;
          return false;
        }
        conf::Papyrus::importDirectories.emplace_back(FSUtils::canonical(d), false);
        if (!conf::General::quietCompile) {
          std::cout << "Adding import directory: " << conf::Papyrus::importDirectories.back().get_unresolved_path()
                    << std::endl;
        }
      }
    }

    if (!filesystem::exists(baseOutputDir))
      filesystem::create_directories(baseOutputDir);
    baseOutputDir = FSUtils::canonicalFS(baseOutputDir);
    conf::General::outputDirectory = baseOutputDir;

    std::string flagsPath;
    if (!flags.empty()) {
      flagsPath = findFlags(flags, progamBasePath, baseOutputDir);
    } else if (conf::Papyrus::game == GameID::Starfield) {
      std::cout << "No flags specified, Using default Starfield flags file." << std::endl;
      flagsPath = "fake://Starfield/Starfield_Papyrus_Flags.flg";
    }

    if (flagsPath.empty()) {
      if (conf::Papyrus::game != GameID::Starfield) {
        std::cout << "Unable to locate flags file '" << flags << "'." << std::endl;
        return false;
      }
      std::cout << "Could not find flags file, using default Starfield flags..." << std::endl;
      flagsPath = "fake://Starfield/Starfield_Papyrus_Flags.flg";
    }
    parseUserFlags(std::move(flagsPath));

    if (!handleImports(conf::Papyrus::importDirectories, jobManager)) {
      std::cout << "Import failed!" << std::endl;
      return false;
    }
    if (!vm["write-config-file"].as<std::string>().empty())
      SaveDefaultConfig(commandLineDesc, vm["write-config-file"].as<std::string>(), vm);

    conf::General::inputFiles.reserve(conf::General::inputFiles.size() + filesPassed.size());

    // PCompiler input resolution
    if (conf::PCompiler::pCompilerCompatibilityMode) {
      for (auto& f : filesPassed) {
        bool isFolder = conf::PCompiler::all;
        if (conf::PCompiler::all) {
          if (!std::filesystem::is_directory(f)) {
            std::cout << "Unable to locate input directory '" << f << "'." << std::endl;
            return false;
          }
        } else {
          // need to replace any `:` with `\`
          std::replace(f.begin(), f.end(), ':', FSUtils::SEP);
          if (FSUtils::extensionAsRef(f).empty())
            f.append(".psc");
        }
        auto & input = conf::General::inputFiles.emplace_back(std::make_shared<PCompInputFile>(f, !iterateCompiledDirectoriesRecursively, isFolder));
      }
    } else {
      // normal resolution
      for (auto& f : filesPassed) {
        if (!filesystem::is_directory(f)) {
          std::string_view ext = FSUtils::extensionAsRef(f);
          if (!pathEq(ext, ".psc") && !pathEq(ext, ".pas") && !pathEq(ext, ".pex") && !pathEq(ext, ".ppj")) {
            std::cout << "Don't know how to handle input file '" << f << "'!" << std::endl;
            std::cout << "Expected either a Papyrus file (*.psc), Pex assembly file (*.pas), or a Pex file (*.pex)!"
                      << std::endl;
            return false;
          }
        }
        auto& input = conf::General::inputFiles.emplace_back(std::make_shared<InputFile>(f, !iterateCompiledDirectoriesRecursively));
      }
    }

    for (auto& input : conf::General::inputFiles) {
      if (!input){
        throw std::runtime_error("Input file is null!");
      }
      if (!input->resolve()) {
        std::cout << "Unable to locate input file '" << input->get_unresolved_path() << "'." << std::endl;
        return false;
      }
      if (std::filesystem::is_directory(input->resolved_absolute())) {
        if (!addFilesFromDirectory(*input,
                                   baseOutputDir,
                                   jobManager,
                                   papyrus::PapyrusCompilationNode::NodeType::PapyrusCompile,
                                   "")) {
          std::cout << "Unable to add input directory '" << input->get_unresolved_path() << "'." << std::endl;
          return false;
        }
      } else if (!addSingleFile(*input,
                                baseOutputDir,
                                jobManager,
                                papyrus::PapyrusCompilationNode::NodeType::PapyrusCompile)) {
        std::cout << "Unable to add input file '" << input->get_unresolved_path() << "'." << std::endl;
        return false;
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
