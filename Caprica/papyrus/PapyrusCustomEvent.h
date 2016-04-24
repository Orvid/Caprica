#pragma once

#include <string>

#include <common/CapricaFileLocation.h>
#include <common/CapricaReportingContext.h>

namespace caprica { namespace papyrus {

struct PapyrusObject;

struct PapyrusCustomEvent final
{
  std::string name{ };
  PapyrusObject* parentObject{ nullptr };
  CapricaFileLocation location;

  explicit PapyrusCustomEvent(CapricaFileLocation loc) : location(loc) { }
  PapyrusCustomEvent(const PapyrusCustomEvent&) = delete;
  ~PapyrusCustomEvent() = default;


};

}}
