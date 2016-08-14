#pragma once

#include <common/CapricaFileLocation.h>
#include <common/CapricaReportingContext.h>
#include <common/identifier_ref.h>
#include <common/IntrusiveLinkedList.h>

#include <papyrus/PapyrusResolutionContext.h>

namespace caprica { namespace papyrus {

struct PapyrusObject;

struct PapyrusCustomEvent final
{
  identifier_ref name{ };
  PapyrusObject* parentObject{ nullptr };
  CapricaFileLocation location;

  explicit PapyrusCustomEvent(CapricaFileLocation loc) : location(loc) { }
  PapyrusCustomEvent(const PapyrusCustomEvent&) = delete;
  ~PapyrusCustomEvent() = default;

  void semantic2(PapyrusResolutionContext* ctx);

private:
  friend IntrusiveLinkedList<PapyrusCustomEvent>;
  PapyrusCustomEvent* next{ nullptr };
};

}}
