#pragma once

#include <vector>

#include <pex/PexString.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexDebugStructOrder final
{
  PexString objectName{ };
  PexString structName{ };
  std::vector<PexString> members{ };

  PexDebugStructOrder() = default;
  ~PexDebugStructOrder() = default;

  void write(PexWriter& wtr) const;
};

}}
