#pragma once

#include <string>

#include <papyrus/PapyrusType.h>

namespace caprica { namespace papyrus {

struct PapyrusLocalVariable final
{
  std::string name{ "" };
  PapyrusType type{ };

  PapyrusLocalVariable() = default;
  ~PapyrusLocalVariable() = default;
};

}}
