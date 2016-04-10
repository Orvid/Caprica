#pragma once

#include <string>
#include <vector>

#include <common/EngineLimits.h>

#include <papyrus/PapyrusFunction.h>
#include <papyrus/PapyrusResolutionContext.h>

#include <pex/PexDebugFunctionInfo.h>
#include <pex/PexFile.h>
#include <pex/PexObject.h>
#include <pex/PexState.h>

namespace caprica { namespace papyrus {

struct PapyrusState final
{
  std::string name{ "" };
  std::vector<PapyrusFunction*> functions{ };

  CapricaFileLocation location;

  explicit PapyrusState(const CapricaFileLocation& loc) : location(loc) { }
  PapyrusState(const PapyrusState&) = delete;
  ~PapyrusState() {
    for (auto f : functions)
      delete f;
  }

  void buildPex(pex::PexFile* file, pex::PexObject* obj) const {
    auto state = new pex::PexState();
    state->name = file->getString(name);

    size_t staticFunctionCount = 0;
    for (auto f : functions) {
      if (f->isGlobal)
        staticFunctionCount++;
      state->functions.push_back(f->buildPex(file, obj, state, pex::PexString()));
    }

    if (name == "") {
      EngineLimits::checkLimit(location, EngineLimits::Type::PexObject_EmptyStateFunctionCount, functions.size(), name.c_str());
      EngineLimits::checkLimit(location, EngineLimits::Type::PexObject_StaticFunctionCount, staticFunctionCount);
    } else {
      EngineLimits::checkLimit(location, EngineLimits::Type::PexState_FunctionCount, functions.size(), name.c_str());
    }

    obj->states.push_back(state);
  }

  void semantic(PapyrusResolutionContext* ctx);
  void semantic2(PapyrusResolutionContext* ctx);
};

}}
