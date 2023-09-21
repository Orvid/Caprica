#pragma once

#include <common/CapricaFileLocation.h>
#include <common/identifier_ref.h>
#include <common/IntrusiveLinkedList.h>

#include <papyrus/PapyrusProperty.h>
#include <papyrus/PapyrusUserFlags.h>

#include <pex/PexDebugPropertyGroup.h>
#include <pex/PexFile.h>
#include <pex/PexObject.h>

namespace caprica { namespace papyrus {

struct PapyrusGuard final
{
  identifier_ref name{ "" };
  const PapyrusObject* parent{ nullptr };

  CapricaFileLocation location;

  explicit PapyrusGuard(CapricaFileLocation loc, const PapyrusObject* par) : location(loc), parent(par) { }
  PapyrusGuard(const PapyrusGuard&) = delete;
  ~PapyrusGuard() = default;

  void buildPex(CapricaReportingContext&, pex::PexFile* file, pex::PexObject* obj) const;
  
  void semantic2(PapyrusResolutionContext* ctx);

private:
  friend IntrusiveLinkedList<PapyrusGuard>;
  PapyrusGuard* next{ nullptr };
};

}}
