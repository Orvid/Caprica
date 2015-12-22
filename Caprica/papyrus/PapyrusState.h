#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusFunction.h>

namespace caprica { namespace papyrus {

struct PapyrusState final
{
  std::string name;
  std::vector<PapyrusFunction*> functions;

  PapyrusState() = default;
  ~PapyrusState() {
    for (auto f : functions)
      delete f;
  }
};

}}
