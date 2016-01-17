#pragma once

#include <vector>

#include <pex/PexReader.h>
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

  explicit PexDebugPropertyGroup() = default;
  ~PexDebugPropertyGroup() = default;

  static PexDebugPropertyGroup* read(PexReader& rdr);
  void write(PexWriter& wtr) const;
};

}}
