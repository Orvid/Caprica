
#include <ostream>

#include <papyrus/PapyrusScript.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexWriter.h>

int main(int argc, char* argv[])
{
  auto a = new caprica::papyrus::PapyrusScript();
  auto pex = a->buildPex();
  std::ofstream strm("test.pex");
  caprica::pex::PexWriter wtr(strm);
  pex->write(wtr);

  return 0;
}

