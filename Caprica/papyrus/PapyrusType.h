#pragma once

#include <string>

namespace caprica { namespace papyrus {

struct PapyrusType final
{
  std::string name{ "" };

  PapyrusType() = default;
  PapyrusType(const PapyrusType& other) = default;

};

}}
