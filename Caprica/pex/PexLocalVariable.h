#pragma once

#include <pex/PexString.h>

namespace caprica { namespace pex {

struct PexWriter;

struct PexLocalVariable final
{
  PexString name{ };
  PexString type{ };

  PexLocalVariable() = default;
  ~PexLocalVariable() = default;

  void write(PexWriter& wtr) const;
};

}}
