#pragma once

#include <string>
#include <vector>

#include <common/CapricaFileLocation.h>
#include <common/CaselessStringComparer.h>
#include <papyrus/PapyrusFunctionParameter.h>
#include <papyrus/PapyrusResolutionContext.h>
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

enum class PapyrusFunctionType
{
  Unknown,

  Getter,
  Setter,
  Function,
  Event,
};

struct PapyrusFunction final
{
  std::string name{ "" };
  std::string documentationComment{ "" };
  PapyrusType returnType;
  PapyrusUserFlags userFlags{ };
  bool isGlobal{ false };
  bool isNative{ false };
  std::vector<PapyrusFunctionParameter*> parameters{ };
  std::vector<statements::PapyrusStatement*> statements{ };
  PapyrusObject* parentObject{ nullptr };
  PapyrusFunctionType functionType{ PapyrusFunctionType::Unknown };

  CapricaFileLocation location;

  PapyrusFunction(const CapricaFileLocation& loc, const PapyrusType& ret) : location(loc), returnType(ret) { }
  ~PapyrusFunction() {
    for (auto p : parameters)
      delete p;
    for (auto s : statements)
      delete s;
  }

  pex::PexFunction* buildPex(pex::PexFile* file,
                             pex::PexObject* obj,
                             pex::PexState* state,
                             pex::PexString propName) const;
  void semantic(PapyrusResolutionContext* ctx);
  void semantic2(PapyrusResolutionContext* ctx);
};

}}
