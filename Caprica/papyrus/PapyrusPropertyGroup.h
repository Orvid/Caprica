#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusProperty.h>
#include <papyrus/PapyrusUserFlags.h>

namespace caprica { namespace papyrus {

struct PapyrusPropertyGroup final
{
  std::string name{ "" };
  std::string documentationComment{ "" };
  PapyrusUserFlags userFlags{ PapyrusUserFlags::None };
  std::vector<PapyrusProperty*> properties{ };

  PapyrusPropertyGroup() = default;
  ~PapyrusPropertyGroup() = default;
};

}}
