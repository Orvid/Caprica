#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusObject.h>
#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus {

struct PapyrusScript final
{
  std::string sourceFileName{ "" };
  std::vector<PapyrusObject*> objects{ };

  PapyrusScript() = default;
  ~PapyrusScript() {
    for (auto obj : objects)
      delete obj;
  }

  pex::PexFile* buildPex() const {
    auto pex = new pex::PexFile();
    pex->debugInfo = new pex::PexDebugInfo();
    pex->sourceFileName = sourceFileName;
    for (auto o : objects)
      o->buildPex(pex);
    return pex;
  }
};

}}