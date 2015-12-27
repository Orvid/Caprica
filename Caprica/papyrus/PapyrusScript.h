#pragma once

#include <boost/filesystem.hpp>

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

  pex::PexFile* buildPex() const {
    auto pex = new pex::PexFile();
    pex->debugInfo = new pex::PexDebugInfo();
    pex->debugInfo->modificationTime = boost::filesystem::last_write_time(sourceFileName);
    pex->compilationTime = time(nullptr);
    pex->sourceFileName = sourceFileName;
    // TODO: Set the computerName and userName as well.
    for (auto o : objects)
      o->buildPex(pex);
    return pex;
  }

  void semantic() {
    auto ctx = new PapyrusResolutionContext();
    ctx->script = this;
    for (auto o : objects)
      o->semantic(ctx);
    delete ctx;
  }
};

}}