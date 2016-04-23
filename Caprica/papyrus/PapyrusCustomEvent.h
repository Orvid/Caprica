#pragma once

#include <string>

#include <common/CapricaError.h>
#include <common/CapricaFileLocation.h>

namespace caprica { namespace papyrus {

struct PapyrusObject;

struct PapyrusCustomEvent final
{
  std::string name{ };
  PapyrusObject* parentObject{ nullptr };
  CapricaFileLocation location;

  explicit PapyrusCustomEvent(const CapricaFileLocation& loc) : location(loc) { }
  PapyrusCustomEvent(const PapyrusCustomEvent&) = delete;
  ~PapyrusCustomEvent() = default;


};

}}
