#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusFunctionParameter.h>
#include <papyrus/PapyrusLocalVariable.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexDebugFunctionInfo.h>
#include <pex/PexFile.h>
#include <pex/PexFunction.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexObject.h>
#include <pex/PexState.h>

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
  std::vector<statements::PapyrusStatement*> statements{ };

  PapyrusFunction() = default;
  ~PapyrusFunction() {
    for (auto p : parameters)
      delete p;
    for (auto s : statements)
      delete s;
  }

  pex::PexFunction* buildPex(pex::PexFile* file,
                             pex::PexObject* obj,
                             pex::PexState* state,
                             pex::PexDebugFunctionType funcType,
                             pex::PexString propName) const;
};

}}
