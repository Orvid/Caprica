#pragma once

#include <string>

#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/PapyrusValue.h>

namespace caprica { namespace papyrus {

struct PapyrusVariable final
{
  std::string name{ "" };
  PapyrusType type{ };
  PapyrusUserFlags userFlags{ PapyrusUserFlags::None };
  PapyrusValue defaultValue{ };
  bool isConst{ false };

  PapyrusVariable() = default;
  ~PapyrusVariable() = default;
};

}}
