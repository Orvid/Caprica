#pragma once

#include <string>

#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/PapyrusValue.h>
#include <papyrus/parser/PapyrusFileLocation.h>

#include <pex/PexFile.h>
#include <pex/PexObject.h>
#include <pex/PexStruct.h>
#include <pex/PexStructMember.h>

namespace caprica { namespace papyrus {

struct PapyrusStructMember final
{
  std::string name{ "" };
  std::string documentationString{ "" };
  PapyrusType type;
  PapyrusUserFlags userFlags{ PapyrusUserFlags::None };
  PapyrusValue defaultValue{ PapyrusValue::Default() };
  bool isConst{ false };

  parser::PapyrusFileLocation location;

  PapyrusStructMember(const parser::PapyrusFileLocation& loc, const PapyrusType& tp) : location(loc), type(tp) { }
  ~PapyrusStructMember() = default;

  void buildPex(pex::PexFile* file, pex::PexObject* obj, pex::PexStruct* struc) const {
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
