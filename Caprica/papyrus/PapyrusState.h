#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusFunction.h>
#include <papyrus/PapyrusResolutionContext.h>

#include <pex/PexDebugFunctionInfo.h>
#include <pex/PexFile.h>
#include <pex/PexObject.h>
#include <pex/PexState.h>

namespace caprica { namespace papyrus {

struct PapyrusState final
{
  std::string name;
  std::vector<PapyrusFunction*> functions;

  PapyrusState() = default;
  ~PapyrusState() {
    for (auto f : functions)
      delete f;
  }

  void buildPex(pex::PexFile* file, pex::PexObject* obj) const {
    auto state = new pex::PexState();
    state->name = file->getString(name);
    for (auto f : functions)
      state->functions.push_back(f->buildPex(file, obj, state, pex::PexDebugFunctionType::Normal, pex::PexString()));
    obj->states.push_back(state);
  }

  void semantic(PapyrusResolutionContext* ctx) {
    ctx->state = this;
    for (auto f : functions)
      f->semantic(ctx);
    ctx->state = nullptr;
  }

  void semantic2(PapyrusResolutionContext* ctx) {
    ctx->state = this;
    for (auto f : functions)
      f->semantic2(ctx);
    ctx->state = nullptr;
  }
};

}}
