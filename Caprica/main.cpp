#include <algorithm>
#include <fstream>
#include <ostream>
#include <string>

#include <ppl.h>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <common/CapricaConfig.h>
#include <common/parser/CapricaUserFlagsParser.h>

#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusScript.h>
#include <papyrus/parser/PapyrusParser.h>

#include <pex/PexAsmWriter.h>
#include <pex/PexReader.h>
#include <pex/PexWriter.h>
#include <pex/parser/PexAsmParser.h>

#include <Windows.h>

boost::filesystem::path naive_uncomplete(boost::filesystem::path const p, boost::filesystem::path const base) {
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

struct ScriptToCompile final
{
  std::string sourceFileName;
  std::string outputDirectory;

  ScriptToCompile() = delete;
  ScriptToCompile(const boost::filesystem::path& sourcePath, const std::string& baseOutputDir, const boost::filesystem::path& relOutputDir) {
    sourceFileName = sourcePath.string();
    outputDirectory = baseOutputDir + "\\" + relOutputDir.string();
  }
  ScriptToCompile(const boost::filesystem::path& sourcePath, const std::string& baseOutputDir) {
    sourceFileName = sourcePath.string();
    outputDirectory = baseOutputDir;
  }
  ~ScriptToCompile() = default;
};

static void compileScript(const ScriptToCompile& script) {
  if (!caprica::CapricaConfig::quietCompile)
    std::cout << "Compiling " << script.sourceFileName << std::endl;
  auto path = boost::filesystem::path(script.sourceFileName);
  auto baseName = boost::filesystem::basename(path.filename());
  auto ext = boost::filesystem::extension(script.sourceFileName);
  if (!_stricmp(ext.c_str(), ".psc")) {
    auto parser = new caprica::papyrus::parser::PapyrusParser(script.sourceFileName);
    auto a = parser->parseScript();
    caprica::CapricaError::exitIfErrors();
    delete parser;
    auto ctx = new caprica::papyrus::PapyrusResolutionContext();
    a->semantic(ctx);
    caprica::CapricaError::exitIfErrors();
    auto pex = a->buildPex();
    caprica::CapricaError::exitIfErrors();
    delete ctx;
    delete a;
    std::ofstream strm(script.outputDirectory + "\\" + baseName + ".pex", std::ofstream::binary);
    caprica::pex::PexWriter wtr(strm);
    pex->write(wtr);

    if (caprica::CapricaConfig::dumpPexAsm) {
      std::ofstream asmStrm(script.outputDirectory + "\\" + baseName + ".pas", std::ofstream::binary);
      caprica::pex::PexAsmWriter asmWtr(asmStrm);
      pex->writeAsm(asmWtr);
    }

    delete pex;
  } else if (!_stricmp(ext.c_str(), ".pas")) {
    auto parser = new caprica::pex::parser::PexAsmParser(script.sourceFileName);
    auto pex = parser->parseFile();
    caprica::CapricaError::exitIfErrors();
    delete parser;
    std::ofstream strm(script.outputDirectory + "\\" + baseName + ".pex", std::ofstream::binary);
    caprica::pex::PexWriter wtr(strm);
    pex->write(wtr);
    delete pex;
  } else if (!_stricmp(ext.c_str(), ".pex")) {
    caprica::pex::PexReader rdr(script.sourceFileName);
    auto pex = caprica::pex::PexFile::read(rdr);
    caprica::CapricaError::exitIfErrors();
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
    return std::make_pair("warning-as-error", str.substr(2));
  else if (str.find("-wd") == 0)
    return std::make_pair("disable-warning", str.substr(2));
  else
    return std::make_pair(std::string(), std::string());
}

static bool parseArgs(int argc, char* argv[], std::vector<ScriptToCompile>& filesToCompile) {
  namespace po = boost::program_options;
  namespace conf = caprica::CapricaConfig;

  try {
    bool iterateCompiledDirectoriesRecursively = false;

    po::options_description desc("General");
    desc.add_options()
      ("help,h", "Print usage information.")
      ("import,i", po::value<std::vector<std::string>>()->composing(), "Set the compiler's import directories.")
      ("flags,f", po::value<std::string>(), "Set the file defining the user flags.")
      ("optimize,O", po::bool_switch(&conf::enableOptimizations)->default_value(false), "Enable optimizations.")
      ("output,o", po::value<std::string>()->default_value(boost::filesystem::current_path().string()), "Set the directory to save compiler output to.")
      ("parallel-compile,p", po::bool_switch(&conf::compileInParallel)->default_value(false), "Compile files in parallel.")
      ("recurse,r", po::bool_switch(&iterateCompiledDirectoriesRecursively)->default_value(false), "Recursively compile all scripts in the directories passed.")
      ("dump-asm", po::bool_switch(&conf::dumpPexAsm)->default_value(false), "Dump the PEX assembly code for the input files.")
      ("all-warnings-as-errors", po::bool_switch(&conf::treatWarningsAsErrors)->default_value(false), "Treat all warnings as if they were errors.")
      ("warning-as-error", po::value<std::vector<size_t>>()->composing(), "Treat a specific warning as an error.")
      ("disable-warning", po::value<std::vector<size_t>>()->composing(), "Disable a specific warning.")
      ("config-file", po::value<std::string>()->default_value("caprica.cfg"), "Load additional options from a config file.")
      ("quiet,q", po::bool_switch(&conf::quietCompile)->default_value(false), "Do not report progress, only failures.")
    ;

    po::options_description champollionCompatDesc("Champollion Compatibility");
    champollionCompatDesc.add_options()
      ("champollion-compat", po::value<bool>()->default_value(true)->implicit_value(true), "Enable a few options that make it easier to compile Papyrus code decompiled by Champollion.")
      ("allow-compiler-identifiers", po::bool_switch(&conf::allowCompilerIdentifiers)->default_value(false), "Allow identifiers to be prefixed with ::, which is normally reserved for compiler identifiers.")
      ("allow-decompiled-struct-references", po::bool_switch(&conf::allowDecompiledStructNameRefs)->default_value(false), "Allow the name of structs to be prefixed with the containing object name, followed by a #, then the name of the struct.")
    ;

    po::options_description advancedDesc("Advanced");
    advancedDesc.add_options()
      ("enable-ck-optimizations", po::value<bool>(&conf::enableCKOptimizations)->default_value(true), "Enable optimizations that the CK compiler normally does regardless of the -optimize switch.")
      ("enable-debug-info", po::value<bool>(&conf::emitDebugInfo)->default_value(true), "Enable the generation of debug info. Disabling this will result in Property Groups not showing up in the Creation Kit for the compiled script. This also removes the line number and struct order information.")
      ("enable-language-extensions", po::value<bool>(&conf::enableLanguageExtensions)->default_value(true), "Enable Caprica's extensions to the Papyrus language.")
      ("enable-speculative-syntax", po::value<bool>(&conf::enableSpeculativeSyntax)->default_value(true), "Enable the speculated syntax for the new Papyrus features in Fallout 4.")
    ;

    po::options_description hiddenDesc("");
    hiddenDesc.add_options()
      ("input-file", po::value<std::vector<std::string>>(), "The input file.")
    ;

    po::positional_options_description p;
    p.add("input-file", -1);

    po::options_description visibleDesc("");
    visibleDesc.add(desc).add(champollionCompatDesc).add(advancedDesc);

    po::options_description commandLineDesc("");
    commandLineDesc.add(visibleDesc).add(hiddenDesc);

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
      std::cout << "Caprica Papyrus Compiler v0.1.4" << std::endl;
      std::cout << "Usage: Caprica <sourceFile / directory>" << std::endl;
      std::cout << "Note that when passing a directory, only Papyrus script files (*.psc) in it will be compiled. Pex (*.pex) and Pex assembly (*.pas) files will be ignored." << std::endl;
      std::cout << visibleDesc << std::endl;
      return false;
    }

    if (vm["champollion-compat"].as<bool>()) {
      conf::allowCompilerIdentifiers = true;
      conf::allowDecompiledStructNameRefs = true;
    }

    if (vm.count("warning-as-error")) {
      auto warnsAsErrs = vm["warning-as-error"].as<std::vector<size_t>>();
      conf::warningsToHandleAsErrors.reserve(warnsAsErrs.size());
      for (auto f : warnsAsErrs) {
        if (conf::warningsToHandleAsErrors.count(f)) {
          std::cout << "Warning " << f << " was already marked as an error." << std::endl;
          return false;
        }
        conf::warningsToHandleAsErrors.insert(f);
      }
    }

    if (vm.count("disable-warning")) {
      auto warnsIgnored = vm["disable-warning"].as<std::vector<size_t>>();
      conf::warningsToIgnore.reserve(warnsIgnored.size());
      for (auto f : warnsIgnored) {
        if (conf::warningsToIgnore.count(f)) {
          std::cout << "Warning " << f << " was already disabled." << std::endl;
          return false;
        }
        conf::warningsToIgnore.insert(f);
      }
    }

    if (vm.count("import")) {
      auto dirs = vm["import"].as<std::vector<std::string>>();
      conf::importDirectories.reserve(dirs.size());
      for (auto d : dirs) {
        if (!boost::filesystem::exists(d)) {
          std::cout << "Unable to find the import directory '" << d << "'!" << std::endl;
          return false;
        }
        conf::importDirectories.push_back(boost::filesystem::canonical(boost::filesystem::absolute(d)).make_preferred().string());
      }
    }

    auto baseOutputDir = vm["output"].as<std::string>();
    if (!boost::filesystem::exists(baseOutputDir))
      boost::filesystem::create_directories(baseOutputDir);
    baseOutputDir = boost::filesystem::canonical(boost::filesystem::absolute(baseOutputDir)).make_preferred().string();

    if (vm.count("flags")) {
      const auto findFlags = [progamBasePath, baseOutputDir](const std::string& flagsPath) -> std::string {
        if (boost::filesystem::exists(flagsPath))
          return flagsPath;

        for (auto& i : conf::importDirectories) {
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

      auto parser = new caprica::parser::CapricaUserFlagsParser(flagsPath);
      parser->parseUserFlags(conf::userFlagsDefinition);
      delete parser;
    }


    auto filesPassed = vm["input-file"].as<std::vector<std::string>>();
    filesToCompile.reserve(filesPassed.size());
    for (auto f : filesPassed) {
      if (!boost::filesystem::exists(f)) {
        std::cout << "Unable to locate input file '" << f << "'." << std::endl;
        return false;
      }
      if (boost::filesystem::is_directory(f)) {
        if (iterateCompiledDirectoriesRecursively) {
          auto absBaseDir = boost::filesystem::canonical(boost::filesystem::absolute(f)).make_preferred();
          boost::system::error_code ec;
          for (auto e : boost::filesystem::recursive_directory_iterator(f, ec)) {
            if (e.path().extension().string() == ".psc") {
              auto abs = boost::filesystem::canonical(boost::filesystem::absolute(e.path())).make_preferred();
              auto rel = naive_uncomplete(abs, absBaseDir).make_preferred();
              
              if (rel.string() != e.path().filename())
                filesToCompile.push_back(ScriptToCompile(e.path(), baseOutputDir, rel.parent_path()));
              else
                filesToCompile.push_back(ScriptToCompile(e.path(), baseOutputDir));
            }
          }
        } else {
          boost::system::error_code ec;
          for (auto e : boost::filesystem::directory_iterator(f, ec)) {
            if (e.path().extension().string() == ".psc")
              filesToCompile.push_back(ScriptToCompile(e.path(), baseOutputDir));
          }
        }
      } else {
        auto ext = boost::filesystem::extension(f);
        if (_stricmp(ext.c_str(), ".psc") && _stricmp(ext.c_str(), ".pas") && _stricmp(ext.c_str(), ".pex")) {
          std::cout << "Don't know how to handle input file '" << f << "'!" << std::endl;
          std::cout << "Expected either a Papyrus file (*.psc), Pex assembly file (*.pas), or a Pex file (*.pex)!" << std::endl;
          return false;
        }
        filesToCompile.push_back(ScriptToCompile(f, baseOutputDir));
      }
    }
  } catch (const std::exception& ex) {
    if (ex.what() != "")
      std::cout << ex.what() << std::endl;
    return false;
  }

  return true;
}

static void breakIfDebugging() {
  if (IsDebuggerPresent()) {
    __debugbreak();
  }
}

int main(int argc, char* argv[])
{
  std::vector<ScriptToCompile> filesToCompile;
  if (!parseArgs(argc, argv, filesToCompile)) {
    breakIfDebugging();
    return -1;
  }

  try {
    if (caprica::CapricaConfig::compileInParallel) {
      concurrency::parallel_for_each(filesToCompile.begin(), filesToCompile.end(), [](const ScriptToCompile& fl) {
        compileScript(fl);
      });
    } else {
      for (auto& file : filesToCompile)
        compileScript(file);
    }
  } catch (const std::runtime_error& ex) {
    if (ex.what() != "")
      std::cout << ex.what() << std::endl;
    breakIfDebugging();
    return -1;
  }

  return 0;
}

