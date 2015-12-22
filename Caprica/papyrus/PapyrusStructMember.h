#pragma once

#include <string>

#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/PapyrusValue.h>

namespace caprica { namespace papyrus {

struct PapyrusStructMember final
{
  std::string name{ "" };
  std::string documentationString{ "" };
  PapyrusType type{ };
  PapyrusUserFlags userFlags{ PapyrusUserFlags::None };
  PapyrusValue defaultValue{ };
  bool isConst{ false };

  PapyrusStructMember() = default;
  ~PapyrusStructMember() = default;
};

}}
