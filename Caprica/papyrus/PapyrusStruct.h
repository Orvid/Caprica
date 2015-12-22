#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusStructMember.h>

namespace caprica { namespace papyrus {

struct PapyrusStruct final
{
  std::string name;
  std::vector<PapyrusStructMember*> members;

  PapyrusStruct() = default;
  ~PapyrusStruct() {
    for (auto m : members)
      delete m;
  }
};

}}
