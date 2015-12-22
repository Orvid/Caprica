#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusProperty.h>
#include <papyrus/PapyrusPropertyGroup.h>
#include <papyrus/PapyrusState.h>
#include <papyrus/PapyrusStruct.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/PapyrusVariable.h>

namespace caprica { namespace papyrus {

struct PapyrusObject final
{
  std::string name{ "" };
  std::string documentationString{ "" };
  bool isConst{ false };
  PapyrusUserFlags userFlags{ PapyrusUserFlags::None };
  PapyrusType parentClass{ };
  PapyrusState* autoState{ nullptr };

  std::vector<PapyrusStruct*> structs{ };
  std::vector<PapyrusVariable*> variables{ };
  std::vector<PapyrusProperty*> properties{ };
  std::vector<PapyrusPropertyGroup*> propertyGroups{ };
  std::vector<PapyrusState*> states{ };

  PapyrusObject() = default;
  ~PapyrusObject() {
    for (auto s : structs)
      delete s;
    for (auto v : variables)
      delete v;
    for (auto p : properties)
      delete p;
    for (auto g : propertyGroups)
      delete g;
    for (auto s : states)
      delete s;
  }
};

}}
