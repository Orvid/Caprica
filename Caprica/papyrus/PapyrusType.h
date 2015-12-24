#pragma once

#include <string>

#include <pex/PexFile.h>
#include <pex/PexString.h>

namespace caprica { namespace papyrus {

struct PapyrusType final
{
  std::string name{ "" };

  PapyrusType() = default;
  PapyrusType(const PapyrusType& other) = default;
  // TODO: Remove this overload; this is only for testing.
  PapyrusType(std::string nm) : name(nm) {  }

  pex::PexString buildPex(pex::PexFile* file) const {
    return file->getString(name);
  }
};

}}
