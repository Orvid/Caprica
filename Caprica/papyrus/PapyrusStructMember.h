#pragma once

#include <string>

#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/PapyrusValue.h>

#include <pex/PexFile.h>
#include <pex/PexObject.h>
#include <pex/PexStructInfo.h>
#include <pex/PexStructMember.h>

namespace caprica { namespace papyrus {

struct PapyrusStructMember final
{
  std::string name{ "" };
  std::string documentationString{ "" };
  PapyrusType type{ };
  PapyrusUserFlags userFlags{ PapyrusUserFlags::None };
  PapyrusValue defaultValue{ };
  bool isConst{ false };

  PapyrusStructMember() = default;
  ~PapyrusStructMember() = default;

  void buildPex(pex::PexFile* file, pex::PexObject* obj, pex::PexStructInfo* struc) const {
    auto member = new pex::PexStructMember();
    member->name = file->getString(name);
    member->documentationString = file->getString(documentationString);
    member->typeName = type.buildPex(file);
    member->userFlags = buildPexUserFlags(file, userFlags);
    member->defaultValue = defaultValue.buildPex(file);
    member->isConst = isConst;
    struc->members.push_back(member);
  }

  void semantic(PapyrusResolutionContext* ctx) {
    type = ctx->resolveType(type);
  }
};

}}
