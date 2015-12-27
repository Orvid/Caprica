#pragma once

#include <string>

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
  PapyrusType type{ };
  PapyrusUserFlags userFlags{ PapyrusUserFlags::None };
  PapyrusValue defaultValue{ };
  bool isConst{ false };

  PapyrusVariable() = default;
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
};

}}
