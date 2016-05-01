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
  RemoteEvent,
};

struct PapyrusFunction final
{
  std::string name{ "" };
  std::string documentationComment{ "" };
  PapyrusType returnType;
  PapyrusUserFlags userFlags{ };
  std::vector<PapyrusFunctionParameter*> parameters{ };
  std::vector<statements::PapyrusStatement*> statements{ };
  PapyrusObject* parentObject{ nullptr };
  PapyrusFunctionType functionType{ PapyrusFunctionType::Unknown };
  std::string remoteEventParent{ "" };
  std::string remoteEventName{ "" };

  CapricaFileLocation location;

  bool isBetaOnly() const;
  bool isDebugOnly() const;
  bool isGlobal() const noexcept { return userFlags.isGlobal; }
  bool isNative() const noexcept { return userFlags.isNative; }

  explicit PapyrusFunction(CapricaFileLocation loc, PapyrusType&& ret) : location(loc), returnType(std::move(ret)) { }
  PapyrusFunction(const PapyrusFunction&) = delete;
  ~PapyrusFunction() {
    for (auto p : parameters)
      delete p;
    for (auto s : statements)
      delete s;
  }

  pex::PexFunction* buildPex(CapricaReportingContext& repCtx, 
                             pex::PexFile* file,
                             pex::PexObject* obj,
                             pex::PexState* state,
                             pex::PexString propName) const;
  void semantic(PapyrusResolutionContext* ctx);
  void semantic2(PapyrusResolutionContext* ctx);

  bool hasSameSignature(const PapyrusFunction* other) const;
  std::string prettySignature() const;
};

}}
