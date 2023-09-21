#pragma once

#include <ctime>

#include <common/IntrusiveLinkedList.h>

#include <pex/PexDebugFunctionInfo.h>
#include <pex/PexDebugPropertyGroup.h>
#include <pex/PexDebugStructOrder.h>
#include <pex/PexReader.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexDebugInfo final
{
  time_t modificationTime{ };
  IntrusiveLinkedList<PexDebugFunctionInfo> functions{ };
  IntrusiveLinkedList<PexDebugPropertyGroup> propertyGroups{ };
  IntrusiveLinkedList<PexDebugStructOrder> structOrders{ };

  explicit PexDebugInfo() = default;
  PexDebugInfo(const PexDebugInfo&) = delete;
  ~PexDebugInfo() = default;

  static PexDebugInfo *read(allocators::ChainedPool *alloc, PexReader &rdr, GameID gameType);
  void write(PexWriter &wtr, GameID gameType) const;
};

}}
