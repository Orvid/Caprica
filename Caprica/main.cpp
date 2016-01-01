
#include <ostream>

#include <boost/filesystem.hpp>

#include <papyrus/parser/PapyrusParser.h>
#include <papyrus/PapyrusScript.h>
#include <pex/PexWriter.h>

int main(int argc, char* argv[])
{
  if (argc != 2) {
    printf("Invoke like: caprica.exe myFile.psc");
  }

#ifdef NDEBUG
  try {
#endif
    std::string file = argv[1];
    auto parser = new caprica::papyrus::parser::PapyrusParser(file);
    auto a = parser->parseScript();
    delete parser;
    auto ctx = new caprica::papyrus::PapyrusResolutionContext();
    a->semantic(ctx);
    auto pex = a->buildPex();
    delete ctx;
    delete a;
    std::ofstream strm(boost::filesystem::basename(file) + ".pex", std::ofstream::binary);
    caprica::pex::PexWriter wtr(strm);
    pex->write(wtr);
    delete pex;
#ifdef NDEBUG
  } catch (std::runtime_error err) {
    printf("%s", err.what());
    //getchar();
  }
#endif

  return 0;
}

