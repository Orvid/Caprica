#pragma once

#include <vector>

#include <pex/PexFunction.h>
#include <pex/PexString.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexState final
{
  PexString name{ };
  std::vector<PexFunction*> functions{ };

  PexState() = default;
  ~PexState() {
    for (auto f : functions)
      delete f;
  }

  void write(PexWriter& wtr) const;
};

}}
