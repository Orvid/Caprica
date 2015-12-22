#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusObject.h>
#include <pex/PexFile.h>

namespace caprica { namespace papyrus {

struct PapyrusScript final
{
  std::string name{ "" };
  std::vector<PapyrusObject*> objects{ };

  PapyrusScript() = default;
  ~PapyrusScript() {
    for (auto obj : objects)
      delete obj;
  }

  pex::PexFile buildPex() {
    return pex::PexFile();
  }
};

}}