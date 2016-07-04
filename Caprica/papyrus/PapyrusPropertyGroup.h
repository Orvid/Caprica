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
  boost::string_ref name{ "" };
  boost::string_ref documentationComment{ "" };
  PapyrusUserFlags userFlags{ };
  std::vector<PapyrusProperty*> properties{ };

  CapricaFileLocation location;

  explicit PapyrusPropertyGroup(CapricaFileLocation loc) : location(loc) { }
  PapyrusPropertyGroup(const PapyrusPropertyGroup&) = delete;
  ~PapyrusPropertyGroup() = default;

  void buildPex(CapricaReportingContext& repCtx, pex::PexFile* file, pex::PexObject* obj) const {
    if (file->debugInfo) {
      auto pg = new pex::PexDebugPropertyGroup();
      pg->objectName = obj->name;
      pg->groupName = file->getString(name);
      pg->documentationString = file->getString(documentationComment);
      pg->userFlags = userFlags.buildPex(file);
      for (auto p : properties)
        pg->properties.push_back(file->getString(p->name));
      file->debugInfo->propertyGroups.push_back(pg);
    }

    for (auto p : properties)
      p->buildPex(repCtx, file, obj);
  }

  void semantic(PapyrusResolutionContext* ctx) {
    for (auto p : properties)
      p->semantic(ctx);
  }

  void semantic2(PapyrusResolutionContext* ctx) {
    ctx->ensureNamesAreUnique(properties, "property");
    for (auto p : properties)
      p->semantic2(ctx);
  }
};

}}
