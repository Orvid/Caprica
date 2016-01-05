#pragma once

#include <vector>

#include <pex/PexReader.h>
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

  static PexDebugStructOrder* read(PexReader& rdr);
  void write(PexWriter& wtr) const;
};

}}
