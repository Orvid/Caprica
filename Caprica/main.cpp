
#include <ostream>
#include <string>

#include <boost/filesystem.hpp>

#include <papyrus/parser/PapyrusParser.h>
#include <papyrus/PapyrusScript.h>
#include <pex/PexWriter.h>

void compileScript(std::string filename) {
  auto parser = new caprica::papyrus::parser::PapyrusParser(filename);
  auto a = parser->parseScript();
  delete parser;
  auto ctx = new caprica::papyrus::PapyrusResolutionContext();
  a->semantic(ctx);
  auto pex = a->buildPex();
  delete ctx;
  delete a;
  std::ofstream strm(boost::filesystem::basename(filename) + ".pex", std::ofstream::binary);
  caprica::pex::PexWriter wtr(strm);
  pex->write(wtr);
  delete pex;
}

int main(int argc, char* argv[])
{
  if (argc != 2) {
    printf("Invoke like: caprica.exe myFile.psc");
  }

#ifdef NDEBUG
  try {
#endif
    std::string file = argv[1];
    if (boost::filesystem::is_directory(file)) {
      boost::system::error_code ec;
      for (auto e : boost::filesystem::directory_iterator(file, ec)) {
        if (boost::filesystem::extension(e.path()) == ".psc") {
          compileScript(e.path().string());
        }
      }
    } else {
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

