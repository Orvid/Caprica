#pragma once

#include <pex/PexString.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexFunctionParameter final
{
  PexString name{ };
  PexString type{ };

  PexFunctionParameter() = default;
  ~PexFunctionParameter() = default;

  void write(PexWriter& wtr) const;
};

}}
