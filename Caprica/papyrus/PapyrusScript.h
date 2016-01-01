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

  PapyrusScript() = default;
  ~PapyrusScript() {
    for (auto obj : objects)
      delete obj;
  }

  pex::PexFile* buildPex() const;

  void semantic() {
    auto ctx = new PapyrusResolutionContext();
    ctx->script = this;
    for (auto o : objects)
      o->semantic(ctx);
    delete ctx;
  }
};

}}