#pragma once

#include <string>

#include <common/CapricaError.h>
#include <common/CapricaFileLocation.h>

namespace caprica { namespace papyrus {

struct PapyrusCustomEvent final
{
  std::string name{ };
  CapricaFileLocation location;

  explicit PapyrusCustomEvent(const CapricaFileLocation& loc) : location(loc) { }
  PapyrusCustomEvent(const PapyrusCustomEvent&) = delete;
  ~PapyrusCustomEvent() = default;


};

}}
