#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusStructMember.h>

#include <pex/PexDebugStructOrder.h>
#include <pex/PexFile.h>
#include <pex/PexObject.h>
#include <pex/PexStructInfo.h>

namespace caprica { namespace papyrus {

struct PapyrusStruct final
{
  std::string name;
  std::vector<PapyrusStructMember*> members;

  PapyrusStruct() = default;
  ~PapyrusStruct() {
    for (auto m : members)
      delete m;
  }

  void buildPex(pex::PexFile* file, pex::PexObject* obj) const {
    auto struc = new pex::PexStructInfo();
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
};

}}
