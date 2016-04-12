#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusObject.h>
#include <papyrus/PapyrusResolutionContext.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus {

struct PapyrusScript final
{
  std::string sourceFileName{ "" };
  std::vector<PapyrusObject*> objects{ };

  explicit PapyrusScript() = default;
  PapyrusScript(const PapyrusScript&) = delete;
  ~PapyrusScript() {
    for (auto obj : objects)
      delete obj;
  }

  pex::PexFile* buildPex() const;

  void preSemantic(PapyrusResolutionContext* ctx) {
    ctx->script = this;
    for (auto o : objects)
      o->preSemantic(ctx);
    ctx->script = nullptr;
  }

  void semantic(PapyrusResolutionContext* ctx) {
    ctx->script = this;
    for (auto o : objects)
      o->semantic(ctx);
    ctx->script = nullptr;
  }
};

}}