#pragma once

#include <ctime>
#include <vector>

#include <pex/PexDebugFunctionInfo.h>
#include <pex/PexDebugPropertyGroup.h>
#include <pex/PexDebugStructOrder.h>
#include <pex/PexReader.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexDebugInfo final
{
  time_t modificationTime{ };
  std::vector<PexDebugFunctionInfo*> functions{ };
  std::vector<PexDebugPropertyGroup*> propertyGroups{ };
  std::vector<PexDebugStructOrder*> structOrders{ };

  PexDebugInfo() = default;
  ~PexDebugInfo() {
    for (auto f : functions)
      delete f;
    for (auto p : propertyGroups)
      delete p;
    for(auto s : structOrders)
      delete s;
  }

  static PexDebugInfo* read(PexReader& rdr);
  void write(PexWriter& wtr) const;
};

}}
