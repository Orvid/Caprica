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

// Generates GetState, which every Skyrim script has
static pex::PexFunction* makeGetState(CapricaReportingContext& repCtx, pex::PexFile* file, pex::PexObject* obj) {
  auto fDebInfo = file->alloc->make<pex::PexDebugFunctionInfo>();
  fDebInfo->objectName = obj->name;
  fDebInfo->functionType = pex::PexDebugFunctionType::Normal;
  fDebInfo->stateName = file->getString("");
  fDebInfo->functionName = file->getString("GetState");
  auto getState = file->alloc->make<pex::PexFunction>();
  getState->name = file->getString("GetState");
  getState->documentationString = file->getString("Function that returns the current state");
  getState->returnTypeName = file->getString("String");
  getState->userFlags = pex::PexUserFlags();
  getState->isGlobal = false;
  getState->isNative = false;
  CapricaFileLocation loc(0);
  pex::PexFunctionBuilder bldr { repCtx, loc, file };
  bldr << pex::op::ret { pex::PexValue::Identifier(file->getString("::State")) };
  bldr.populateFunction(getState, fDebInfo);
  return getState;
}

// Generates GotoState, which every Skyrim script has
static pex::PexFunction* makeGotoState(CapricaReportingContext& repCtx, pex::PexFile* file, pex::PexObject* obj) {
  auto fDebInfo = file->alloc->make<pex::PexDebugFunctionInfo>();
  fDebInfo->objectName = obj->name;
  fDebInfo->functionType = pex::PexDebugFunctionType::Normal;
  fDebInfo->stateName = file->getString("");
  fDebInfo->functionName = file->getString("GotoState");
  auto gotoState = file->alloc->make<pex::PexFunction>();
  gotoState->name = file->getString("GotoState");
  gotoState->documentationString = file->getString("Function that switches this object to the specified state");
  gotoState->returnTypeName = file->getString("None");
  gotoState->userFlags = pex::PexUserFlags();
  gotoState->isGlobal = false;
  gotoState->isNative = false;
  auto newState = file->alloc->make<pex::PexFunctionParameter>();
  newState->name = file->getString("newState");
  newState->type = file->getString("String");
  gotoState->parameters.push_back(newState);
  auto selfstring = file->getString("self");
  CapricaFileLocation loc(0);
  pex::PexFunctionBuilder bldr { repCtx, loc, file };
  bldr << pex::op::callmethod { file->getString("onEndState"),
                                pex::PexValue::Identifier(selfstring),
                                pex::PexValue::Identifier { bldr.getNoneLocal(loc) },
                                {} }
       << pex::op::assign { pex::PexValue::Identifier(file->getString("::State")),
                            pex::PexValue::Identifier(newState->name) }
       << pex::op::callmethod { file->getString("onBeginState"),
                                pex::PexValue::Identifier(selfstring),
                                pex::PexValue::Identifier { bldr.getNoneLocal(loc) },
                                {} };
  bldr.populateFunction(gotoState, fDebInfo);
  return gotoState;
};

struct PapyrusState final {
  identifier_ref name { "" };
  caseless_unordered_identifier_ref_map<PapyrusFunction*> functions {};

  CapricaFileLocation location;

  explicit PapyrusState(CapricaFileLocation loc) : location(loc) { }
  PapyrusState(const PapyrusState&) = delete;
  ~PapyrusState() = default;

  void buildPex(CapricaReportingContext& repCtx, pex::PexFile* file, pex::PexObject* obj) const {
    auto state = file->alloc->make<pex::PexState>();
    state->name = file->getString(name);

    // Every skyrim script has these exact same functions
    if (file->gameID == GameID::Skyrim && name == "") {
      state->functions.push_back(makeGetState(repCtx, file, obj));
      state->functions.push_back(makeGotoState(repCtx, file, obj));
    }

    size_t staticFunctionCount = 0;
    for (auto& f : functions) {
      if (f.second->isGlobal())
        staticFunctionCount++;
      state->functions.push_back(f.second->buildPex(repCtx, file, obj, state, pex::PexString()));
    }

    if (name == "") {
      EngineLimits::checkLimit(repCtx,
                               location,
                               EngineLimits::Type::PexObject_EmptyStateFunctionCount,
                               functions.size(),
                               name);
      EngineLimits::checkLimit(repCtx,
                               location,
                               EngineLimits::Type::PexObject_StaticFunctionCount,
                               staticFunctionCount);
    } else {
      EngineLimits::checkLimit(repCtx, location, EngineLimits::Type::PexState_FunctionCount, functions.size(), name);
    }

    obj->states.push_back(state);
  }

  void semantic(PapyrusResolutionContext* ctx);
  void semantic2(PapyrusResolutionContext* ctx);

private:
  friend IntrusiveLinkedList<PapyrusState>;
  PapyrusState* next { nullptr };
};

}}
