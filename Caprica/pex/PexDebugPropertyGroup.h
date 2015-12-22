#pragma once

#include <vector>

#include <pex/PexString.h>
#include <pex/PexUserFlags.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexDebugPropertyGroup final
{
  PexString objectName{ };
  PexString groupName{ };
  PexString documentationString{ };
  PexUserFlags userFlags{ };
  std::vector<PexString> properties{ };

  PexDebugPropertyGroup() = default;
  ~PexDebugPropertyGroup() = default;

  void write(PexWriter& wtr) const;
};

}}
