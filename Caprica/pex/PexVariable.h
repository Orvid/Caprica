#pragma once

#include <pex/PexString.h>
#include <pex/PexUserFlags.h>
#include <pex/PexValue.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexVariable final
{
  PexString name{ };
  PexString typeName{ };
  PexUserFlags userFlags{ };
  PexValue defaultValue{ };
  bool isConst{ false };

  PexVariable() = default;
  ~PexVariable() = default;

  void write(PexWriter& wtr) const;
};

}}
