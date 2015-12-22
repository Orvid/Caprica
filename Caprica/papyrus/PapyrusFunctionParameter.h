#pragma once

#include <string>

#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusValue.h>

namespace caprica { namespace papyrus {

struct PapyrusFunctionParameter final
{
  std::string name{ "" };
  PapyrusType type{ };
  PapyrusValue defaultValue{ };

  PapyrusFunctionParameter() = default;
  ~PapyrusFunctionParameter() = default;
};

}}
