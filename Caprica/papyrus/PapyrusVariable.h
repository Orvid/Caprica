#pragma once

#include <string>

#include <common/CapricaFileLocation.h>
#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/PapyrusValue.h>

#include <pex/PexFile.h>
#include <pex/PexObject.h>
#include <pex/PexVariable.h>

namespace caprica { namespace papyrus {

struct PapyrusVariable final
{
  std::string name{ "" };
  PapyrusType type;
  PapyrusUserFlags userFlags{ PapyrusUserFlags::None };
  PapyrusValue defaultValue{ PapyrusValue::Default() };
  bool isConst{ false };

  CapricaFileLocation location;

  PapyrusVariable(const CapricaFileLocation& loc, const PapyrusType& tp) : location(loc), type(tp) { }
  ~PapyrusVariable() = default;

  void buildPex(pex::PexFile* file, pex::PexObject* obj) const {
    auto var = new pex::PexVariable();
    var->name = file->getString(name);
    var->typeName = type.buildPex(file);
    var->userFlags = buildPexUserFlags(file, userFlags);
    var->defaultValue = defaultValue.buildPex(file);
    var->isConst = isConst;
    obj->variables.push_back(var);
  }

  void semantic(PapyrusResolutionContext* ctx) {
    type = ctx->resolveType(type);
    defaultValue = PapyrusResolutionContext::coerceDefaultValue(defaultValue, type);
  }
};

}}
