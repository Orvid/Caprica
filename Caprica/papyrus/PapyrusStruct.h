#pragma once

#include <string>
#include <vector>

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
  std::string name{ "" };
  std::vector<PapyrusStructMember*> members{ };
  PapyrusObject* parentObject{ nullptr };

  CapricaFileLocation location;

  PapyrusStruct(const CapricaFileLocation& loc) : location(loc) { }
  ~PapyrusStruct() {
    for (auto m : members)
      delete m;
  }

  void buildPex(pex::PexFile* file, pex::PexObject* obj) const {
    auto struc = new pex::PexStruct();
    struc->name = file->getString(name);

    auto debInf = new pex::PexDebugStructOrder();
    debInf->objectName = obj->name;
    debInf->structName = struc->name;

    for (auto m : members) {
      m->buildPex(file, obj, struc);
      debInf->members.push_back(file->getString(m->name));
    }
    obj->structs.push_back(struc);
    
    if (file->debugInfo)
      file->debugInfo->structOrders.push_back(debInf);
    else
      delete debInf;
  }

  void semantic(PapyrusResolutionContext* ctx) {
    PapyrusResolutionContext::ensureNamesAreUnique(members, "member");
    for (auto m : members)
      m->semantic(ctx);
  }
};

}}
