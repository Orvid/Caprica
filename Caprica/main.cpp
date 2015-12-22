
#include <papyrus/PapyrusScript.h>
#include <pex/PexWriter.h>

int main(int argc, char* argv[])
{
  auto a = new caprica::papyrus::PapyrusScript();
  auto pex = a->buildPex();
  caprica::pex::PexWriter wtr("test.pex");
  pex.writeToFile(wtr);

  return 0;
}

