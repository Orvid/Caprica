#pragma once

#include <common/identifier_ref.h>
#include <common/IntrusiveLinkedList.h>

#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusStructMember.h>

#include <pex/PexDebugStructOrder.h>
#include <pex/PexFile.h>
#include <pex/PexObject.h>
#include <pex/PexStruct.h>

namespace caprica { namespace papyrus {

struct PapyrusObject;

struct PapyrusStruct final
{
  identifier_ref name{ "" };
  IntrusiveLinkedList<PapyrusStructMember> members{ };
  PapyrusObject* parentObject{ nullptr };

  CapricaFileLocation location;

  explicit PapyrusStruct(CapricaFileLocation loc) : location(loc) { }
  PapyrusStruct(const PapyrusStruct&) = delete;
  ~PapyrusStruct() = default;

  void buildPex(CapricaReportingContext& repCtx, pex::PexFile* file, pex::PexObject* obj) const {
    auto struc = file->alloc->make<pex::PexStruct>();
    struc->name = file->getString(name);

    auto debInf = file->alloc->make<pex::PexDebugStructOrder>();
    debInf->objectName = obj->name;
    debInf->structName = struc->name;

    for (auto m : members) {
      m->buildPex(repCtx, file, obj, struc);
      debInf->members.push_back(file->alloc->make<pex::IntrusivePexString>(file->getString(m->name)));
    }
    obj->structs.push_back(struc);
    
    if (file->debugInfo)
      file->debugInfo->structOrders.push_back(debInf);
  }

  void semantic(PapyrusResolutionContext* ctx) {
    ctx->ensureNamesAreUnique(members, "member");
    for (auto m : members)
      m->semantic(ctx);
  }

private:
  friend IntrusiveLinkedList<PapyrusStruct>;
  PapyrusStruct* next{ nullptr };
};

}}
