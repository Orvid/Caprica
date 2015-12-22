#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusFunctionParameter.h>
#include <papyrus/PapyrusLocalVariable.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/statements/PapyrusStatement.h>

namespace caprica { namespace papyrus {

struct PapyrusFunction final
{
  std::string name{ "" };
  std::string documentationComment{ "" };
  PapyrusType returnType{ };
  PapyrusUserFlags userFlags{ PapyrusUserFlags::None };
  bool isGlobal{ false };
  bool isNative{ false };
  std::vector<PapyrusFunctionParameter*> parameters{ };
  std::vector<PapyrusLocalVariable*> locals{ };
  std::vector<statements::PapyrusStatement*> statements{ };

  PapyrusFunction() = default;
  ~PapyrusFunction() {
    for (auto p : parameters)
      delete p;
    for (auto l : locals)
      delete l;
    for (auto s : statements)
      delete s;
  }
};

}}
