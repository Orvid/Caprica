#pragma once

#include <common/CaselessStringComparer.h>
#include <common/EngineLimits.h>
#include <common/identifier_ref.h>
#include <common/IntrusiveLinkedList.h>

#include <papyrus/PapyrusFunction.h>
#include <papyrus/PapyrusResolutionContext.h>

#include <pex/PexDebugFunctionInfo.h>
#include <pex/PexFile.h>
#include <pex/PexObject.h>
#include <pex/PexState.h>

namespace caprica { namespace papyrus {

struct PapyrusState final
{
  identifier_ref name{ "" };
  caseless_unordered_identifier_ref_map<PapyrusFunction*> functions{ };

  CapricaFileLocation location;

  explicit PapyrusState(CapricaFileLocation loc) : location(loc) { }
  PapyrusState(const PapyrusState&) = delete;
  ~PapyrusState() = default;

  void buildPex(CapricaReportingContext& repCtx, pex::PexFile* file, pex::PexObject* obj) const {
    auto state = file->alloc->make<pex::PexState>();
    state->name = file->getString(name);

    size_t staticFunctionCount = 0;
    for (auto& f : functions) {
      if (f.second->isGlobal())
        staticFunctionCount++;
      state->functions.push_back(f.second->buildPex(repCtx, file, obj, state, pex::PexString()));
    }

    if (name == "") {
      EngineLimits::checkLimit(repCtx, location, EngineLimits::Type::PexObject_EmptyStateFunctionCount, functions.size(), name);
      EngineLimits::checkLimit(repCtx, location, EngineLimits::Type::PexObject_StaticFunctionCount, staticFunctionCount);
    } else {
      EngineLimits::checkLimit(repCtx, location, EngineLimits::Type::PexState_FunctionCount, functions.size(), name);
    }

    obj->states.push_back(state);
  }

  void semantic(PapyrusResolutionContext* ctx);
  void semantic2(PapyrusResolutionContext* ctx);

private:
  friend IntrusiveLinkedList<PapyrusState>;
  PapyrusState* next{ nullptr };
};

}}
