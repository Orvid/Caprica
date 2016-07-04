#pragma once

#include <string>

#include <common/CapricaFileLocation.h>
#include <common/CapricaReportingContext.h>

#include <papyrus/PapyrusResolutionContext.h>

namespace caprica { namespace papyrus {

struct PapyrusObject;

struct PapyrusCustomEvent final
{
  boost::string_ref name{ };
  PapyrusObject* parentObject{ nullptr };
  CapricaFileLocation location;

  explicit PapyrusCustomEvent(CapricaFileLocation loc) : location(loc) { }
  PapyrusCustomEvent(const PapyrusCustomEvent&) = delete;
  ~PapyrusCustomEvent() = default;

  void semantic2(PapyrusResolutionContext* ctx);
};

}}
