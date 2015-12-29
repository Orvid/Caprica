#pragma once

#include <string>

#include <papyrus/PapyrusIdentifier.h>
#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusValue.h>

#include <pex/PexFile.h>
#include <pex/PexFunction.h>
#include <pex/PexFunctionParameter.h>
#include <pex/PexObject.h>
#include <pex/PexState.h>

namespace caprica { namespace papyrus {

struct PapyrusFunctionParameter final
{
  std::string name{ "" };
  PapyrusType type{ };
  PapyrusValue defaultValue{ };

  PapyrusFunctionParameter() = default;
  ~PapyrusFunctionParameter() = default;

  void buildPex(pex::PexFile* file, pex::PexObject* obj, pex::PexFunction* func) const {
    auto param = new pex::PexFunctionParameter();
    param->name = file->getString(name);
    param->type = type.buildPex(file);
    func->parameters.push_back(param);
  }
  
  void semantic(PapyrusResolutionContext* ctx) {
    type = ctx->resolveType(type);

    PapyrusIdentifier id;
    id.type = PapyrusIdentifierType::Parameter;
    id.name = name;
    id.param = this;
    ctx->addIdentifier(id);
  }
};

}}
