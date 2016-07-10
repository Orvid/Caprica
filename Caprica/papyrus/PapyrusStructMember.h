#pragma once

#include <string>

#include <common/CapricaFileLocation.h>
#include <common/IntrusiveLinkedList.h>

#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/PapyrusValue.h>

#include <pex/PexFile.h>
#include <pex/PexObject.h>
#include <pex/PexStruct.h>
#include <pex/PexStructMember.h>

namespace caprica { namespace papyrus {

struct PapyrusStructMember final
{
  boost::string_ref name{ "" };
  boost::string_ref documentationString{ "" };
  PapyrusType type;
  PapyrusUserFlags userFlags{ };
  PapyrusValue defaultValue{ PapyrusValue::Default() };

  CapricaFileLocation location;
  const PapyrusStruct* parent{ nullptr };

  bool isConst() const { return userFlags.isConst; }

  explicit PapyrusStructMember(CapricaFileLocation loc, PapyrusType&& tp, const PapyrusStruct* par) : location(loc), type(std::move(tp)), parent(par) { }
  PapyrusStructMember(const PapyrusStructMember&) = delete;
  ~PapyrusStructMember() = default;

  void buildPex(CapricaReportingContext& repCtx, pex::PexFile* file, pex::PexObject* obj, pex::PexStruct* struc) const {
    auto member = new pex::PexStructMember();
    member->name = file->getString(name);
    member->documentationString = file->getString(documentationString);
    member->typeName = type.buildPex(file);
    member->userFlags = userFlags.buildPex(file);
    member->defaultValue = defaultValue.buildPex(file);
    member->isConst = isConst();
    struc->members.push_back(member);
  }

  void semantic(PapyrusResolutionContext* ctx) {
    type = ctx->resolveType(type, true);
    if (type.type == PapyrusType::Kind::Array)
      ctx->reportingContext.error(type.location, "Struct members are not allowed to be arrays.");
    else if (type.type == PapyrusType::Kind::ResolvedStruct)
      ctx->reportingContext.error(type.location, "Struct members are not allowed to be other structs.");
    else if (type.type == PapyrusType::Kind::Var)
      ctx->reportingContext.error(type.location, "Struct members are not allowed to be var.");
    defaultValue = ctx->coerceDefaultValue(defaultValue, type);
  }

private:
  friend IntrusiveLinkedList<PapyrusStructMember>;
  PapyrusStructMember* next{ nullptr };
};

}}
