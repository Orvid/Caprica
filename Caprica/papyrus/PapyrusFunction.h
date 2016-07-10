#pragma once

#include <string>
#include <vector>

#include <common/CapricaFileLocation.h>
#include <common/CaselessStringComparer.h>
#include <common/IntrusiveLinkedList.h>

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
  boost::string_ref name{ "" };
  boost::string_ref documentationComment{ "" };
  PapyrusType returnType;
  PapyrusUserFlags userFlags{ };
  std::vector<PapyrusFunctionParameter*> parameters{ };
  std::vector<statements::PapyrusStatement*> statements{ };
  PapyrusObject* parentObject{ nullptr };
  PapyrusFunctionType functionType{ PapyrusFunctionType::Unknown };
  boost::string_ref remoteEventParent{ "" };
  boost::string_ref remoteEventName{ "" };

  CapricaFileLocation location;

  bool isBetaOnly() const;
  bool isDebugOnly() const;
  bool isGlobal() const noexcept { return userFlags.isGlobal; }
  bool isNative() const noexcept { return userFlags.isNative; }

  explicit PapyrusFunction(CapricaFileLocation loc, PapyrusType&& ret) : location(loc), returnType(std::move(ret)) { }
  PapyrusFunction(const PapyrusFunction&) = delete;
  ~PapyrusFunction() = default;

  pex::PexFunction* buildPex(CapricaReportingContext& repCtx, 
                             pex::PexFile* file,
                             pex::PexObject* obj,
                             pex::PexState* state,
                             pex::PexString propName) const;
  void semantic(PapyrusResolutionContext* ctx);
  void semantic2(PapyrusResolutionContext* ctx);

  bool hasSameSignature(const PapyrusFunction* other) const;
  std::string prettySignature() const;

private:
  friend IntrusiveLinkedList<PapyrusFunction>;
  PapyrusFunction* next{ nullptr };
};

}}
