#pragma once

#include <string>
#include <vector>

#include <common/CapricaFileLocation.h>
#include <papyrus/PapyrusProperty.h>
#include <papyrus/PapyrusUserFlags.h>

#include <pex/PexDebugPropertyGroup.h>
#include <pex/PexFile.h>
#include <pex/PexObject.h>

namespace caprica { namespace papyrus {

struct PapyrusPropertyGroup final
{
  std::string name{ "" };
  std::string documentationComment{ "" };
  PapyrusUserFlags userFlags{ PapyrusUserFlags::None };
  std::vector<PapyrusProperty*> properties{ };

  CapricaFileLocation location;

  PapyrusPropertyGroup(const CapricaFileLocation& loc) : location(loc) { }
  ~PapyrusPropertyGroup() {
    for (auto p : properties)
      delete p;
  }

  void buildPex(pex::PexFile* file, pex::PexObject* obj) const {
    if (file->debugInfo) {
      auto pg = new pex::PexDebugPropertyGroup();
      pg->objectName = obj->name;
      pg->groupName = file->getString(name);
      pg->documentationString = file->getString(documentationComment);
      pg->userFlags = buildPexUserFlags(file, userFlags);
      for (auto p : properties)
        pg->properties.push_back(file->getString(p->name));
      file->debugInfo->propertyGroups.push_back(pg);
    }

    for (auto p : properties)
      p->buildPex(file, obj);
  }

  void semantic(PapyrusResolutionContext* ctx) {
    PapyrusResolutionContext::ensureNamesAreUnique(properties, "property");
    for (auto p : properties)
      p->semantic(ctx);
  }

  void semantic2(PapyrusResolutionContext* ctx) {
    for (auto p : properties)
      p->semantic2(ctx);
  }
};

}}
