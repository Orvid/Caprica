#pragma once

#include <vector>

#include <pex/PexString.h>
#include <pex/PexStructMember.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexStructInfo final
{
  PexString name{ };
  std::vector<PexStructMember*> members{ };

  PexStructInfo() = default;
  ~PexStructInfo() {
    for (auto m : members)
      delete m;
  }

  void write(PexWriter& wtr) const;
};

}}
