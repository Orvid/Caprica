#include <ostream>
#include <string>

#include <ppl.h>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <common/CapricaConfig.h>

#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusScript.h>
#include <papyrus/parser/PapyrusParser.h>

#include <pex/PexAsmWriter.h>
#include <pex/PexReader.h>
#include <pex/PexWriter.h>
#include <pex/parser/PexAsmParser.h>

void compileScript(std::string filename) {
  std::cout << "Compiling " << filename << std::endl;
  auto path = boost::filesystem::path(filename);
  auto baseName = boost::filesystem::basename(path.filename());
  auto ext = boost::filesystem::extension(filename);
  if (!_stricmp(ext.c_str(), ".psc")) {
    auto parser = new caprica::papyrus::parser::PapyrusParser(filename);
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
    std::ofstream strm(caprica::CapricaConfig::outputDirectory + baseName + ".pex", std::ofstream::binary);
    caprica::pex::PexWriter wtr(strm);
    pex->write(wtr);

    if (caprica::CapricaConfig::dumpPexAsm) {
      std::ofstream asmStrm(caprica::CapricaConfig::outputDirectory + baseName + ".pas", std::ofstream::binary);
      caprica::pex::PexAsmWriter asmWtr(asmStrm);
      pex->writeAsm(asmWtr);
    }

    delete pex;
  } else if (!_stricmp(ext.c_str(), ".pas")) {
    auto parser = new caprica::pex::parser::PexAsmParser(filename);
    auto pex = parser->parseFile();
    caprica::CapricaError::exitIfErrors();
    delete parser;
    std::ofstream strm(caprica::CapricaConfig::outputDirectory + baseName + ".pex", std::ofstream::binary);
    caprica::pex::PexWriter wtr(strm);
    pex->write(wtr);
    delete pex;
  } else if (!_stricmp(ext.c_str(), ".pex")) {
    caprica::pex::PexReader rdr(filename);
    auto pex = caprica::pex::PexFile::read(rdr);
    caprica::CapricaError::exitIfErrors();
    std::ofstream asmStrm(caprica::CapricaConfig::outputDirectory + baseName + ".pas", std::ofstream::binary);
    caprica::pex::PexAsmWriter asmWtr(asmStrm);
    pex->writeAsm(asmWtr);
    delete pex;
  } else {
    std::cout << "Don't know how to compile " << filename << "!" << std::endl;
  }
}

std::pair<std::string, std::string> parseOddArguments(const std::string& str) {
  if (str == "WE")
    return std::make_pair("all-warnings-as-errors", "");
  else if (str.find("we") == 0)
    return std::make_pair("warning-as-error", str.substr(2));
  else if (str.find("wd") == 0)
    return std::make_pair("disable-warning", str.substr(2));
  else
    return std::make_pair(std::string(), std::string());
}

bool parseArgs(int argc, char* argv[], std::vector<std::string>& filesToCompile) {
  namespace po = boost::program_options;
  namespace conf = caprica::CapricaConfig;

  try {
    po::options_description desc("General");
    desc.add_options()
      ("help,h", "Print usage information.")
      ("import,i", po::value<std::vector<std::string>>(&conf::importDirectories), "Set the compiler's import directories.")
      ("optimize,O", po::bool_switch(&conf::enableOptimizations)->default_value(false), "Enable optimizations.")
      ("output,o", po::value<std::string>(&conf::outputDirectory)->default_value(boost::filesystem::absolute("./").string()), "Set the directory to save compiler output to.")
      ("parallel-compile,p", po::bool_switch(&conf::compileInParallel)->default_value(false), "Compile files in parallel.")
      ("dump-asm", po::bool_switch(&conf::dumpPexAsm)->default_value(false), "Dump the PEX assembly code for the input files.")
      ("all-warnings-as-errors", po::bool_switch(&conf::treatWarningsAsErrors)->default_value(false), "Treat all warnings as if they were errors.")
      ("warning-as-error", po::value<std::vector<size_t>>(), "Treat a specific warning as an error.")
      ("disable-warning", po::value<std::vector<size_t>>(), "Disable a specific warning.")
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

    po::options_description visibleDesc("Caprica Papyrus Compiler v0.0.8\nUsage: Caprica <sourceFile/directory>\nNote that when passing a directory, only Papyrus script files (*.psc) in it will be compiled. Pex (*.pex) and Pex assembly (*.pas) files will be ignored.");
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

    if (vm.count("help") || !vm.count("input-file")) {
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

    auto filesPassed = vm["input-file"].as<std::vector<std::string>>();
    filesToCompile.reserve(filesPassed.size());
    for (auto f : filesPassed) {
      if (!boost::filesystem::exists(f)) {
        std::cout << "Unable to locate input file '" << f << "'." << std::endl;
        return false;
      }
      if (boost::filesystem::is_directory(f)) {
        boost::system::error_code ec;
        for (auto e : boost::filesystem::directory_iterator(f, ec)) {
          if (e.path().extension().string() == ".psc")
            filesToCompile.push_back(e.path().string());
        }
      } else {
        auto ext = boost::filesystem::extension(f);
        if (_stricmp(ext.c_str(), ".psc") && _stricmp(ext.c_str(), ".pas") && _stricmp(ext.c_str(), ".pex")) {
          std::cout << "Don't know how to handle input file '" << f << "'!" << std::endl;
          std::cout << "Expected either a Papyrus file (*.psc), Pex assembly file (*.pas), or a Pex file (*.pex)!" << std::endl;
          return false;
        }
        filesToCompile.push_back(f);
      }
    }
  } catch (const std::exception& ex) {
    std::cout << ex.what() << std::endl;
    return false;
  }

  return true;
}

int main(int argc, char* argv[])
{
  std::vector<std::string> filesToCompile;
  if (!parseArgs(argc, argv, filesToCompile)) {
    __debugbreak();
    return -1;
  }

  try {
    if (caprica::CapricaConfig::compileInParallel) {
      concurrency::parallel_for_each(filesToCompile.begin(), filesToCompile.end(), [](std::string fl) {
        compileScript(fl);
      });
    } else {
      for (auto& file : filesToCompile)
        compileScript(file);
    }
  } catch (const std::runtime_error& err) {
    err;
    //std::cout << err.what() << std::endl;
    __debugbreak();
  }

  return 0;
}

