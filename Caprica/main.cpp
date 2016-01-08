#include <ostream>
#include <string>

#include <ppl.h>
#include <boost/filesystem.hpp>

#include <common/CapricaConfig.h>

#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusScript.h>
#include <papyrus/parser/PapyrusParser.h>

#include <pex/PexAsmWriter.h>
#include <pex/PexReader.h>
#include <pex/PexWriter.h>
#include <pex/parser/PexAsmParser.h>

void compileScript(std::string filename) {
  printf("Compiling %s\n", filename.c_str());
  auto baseName = boost::filesystem::basename(filename);
  auto ext = boost::filesystem::extension(filename);
  if (ext == ".psc") {
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
    std::ofstream strm(baseName + ".pex", std::ofstream::binary);
    caprica::pex::PexWriter wtr(strm);
    pex->write(wtr);

    if (caprica::CapricaConfig::dumpPexAsm) {
      std::ofstream asmStrm(baseName + ".pas", std::ofstream::binary);
      caprica::pex::PexAsmWriter asmWtr(asmStrm);
      pex->writeAsm(asmWtr);
    }

    delete pex;
  } else if (ext == ".pas") {
    auto parser = new caprica::pex::parser::PexAsmParser(filename);
    auto pex = parser->parseFile();
    caprica::CapricaError::exitIfErrors();
    delete parser;
    std::ofstream strm(baseName + ".pex", std::ofstream::binary);
    caprica::pex::PexWriter wtr(strm);
    pex->write(wtr);
    delete pex;
  } else if (ext == ".pex") {
    caprica::pex::PexReader rdr(filename);
    auto pex = caprica::pex::PexFile::read(rdr);
    caprica::CapricaError::exitIfErrors();
    std::ofstream asmStrm(baseName + ".pas", std::ofstream::binary);
    caprica::pex::PexAsmWriter asmWtr(asmStrm);
    pex->writeAsm(asmWtr);
    delete pex;
  } else {
    printf("Don't know how to compile %s!\n", filename.c_str());
  }
}

int main(int argc, char* argv[])
{
  if (argc != 2) {
    printf("Invoke like: caprica.exe myFile.psc");
  }

  printf("Caprica Papyrus Compiler v0.0.8\n");

#ifdef NDEBUG
  try {
#endif
    std::string file = argv[1];
    if (boost::filesystem::is_directory(file)) {
      caprica::CapricaConfig::importDirectories.push_back(boost::filesystem::absolute(file).string());
      std::vector<std::string> files;
      boost::system::error_code ec;
      for (auto e : boost::filesystem::directory_iterator(file, ec)) {
        if (boost::filesystem::extension(e.path()) == ".psc" || boost::filesystem::extension(e.path()) == ".pas")
          files.push_back(e.path().string());
      }

      if (caprica::CapricaConfig::compileInParallel) {
        concurrency::parallel_for_each(files.begin(), files.end(), [](std::string fl) {
          compileScript(fl);
        });
      } else {
        for (auto& file : files)
          compileScript(file);
      }
    } else {
      caprica::CapricaConfig::importDirectories.push_back(boost::filesystem::path(boost::filesystem::absolute(file)).parent_path().string());
      compileScript(file);
    }
#ifdef NDEBUG
  } catch (std::runtime_error err) {
    printf("%s", err.what());
    getchar();
  }
#endif

  return 0;
}

