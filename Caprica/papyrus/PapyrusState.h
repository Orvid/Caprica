#pragma once

#include <string>
#include <vector>

#include <common/EngineLimits.h>
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
  boost::string_ref name{ "" };
  IntrusiveLinkedList<PapyrusFunction> functions{ };

  CapricaFileLocation location;

  explicit PapyrusState(CapricaFileLocation loc) : location(loc) { }
  PapyrusState(const PapyrusState&) = delete;
  ~PapyrusState() = default;

  void buildPex(CapricaReportingContext& repCtx, pex::PexFile* file, pex::PexObject* obj) const {
    auto state = new pex::PexState();
    state->name = file->getString(name);

    size_t staticFunctionCount = 0;
    for (auto f : functions) {
      if (f->isGlobal())
        staticFunctionCount++;
      state->functions.push_back(f->buildPex(repCtx, file, obj, state, pex::PexString()));
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
};

}}
