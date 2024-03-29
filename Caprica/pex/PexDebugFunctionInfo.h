#pragma once

#include <ctime>
#include <vector>

#include <common/IntrusiveLinkedList.h>

#include <pex/PexReader.h>
#include <pex/PexString.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

enum class PexDebugFunctionType {
  Normal = 0,
  Getter = 1,
  Setter = 2,
};

struct PexDebugFunctionInfo final {
  PexString objectName {};
  PexString stateName {};
  PexString functionName {};
  PexDebugFunctionType functionType { PexDebugFunctionType::Normal };
  std::vector<uint16_t> instructionLineMap {};

  explicit PexDebugFunctionInfo() = default;
  PexDebugFunctionInfo(const PexDebugFunctionInfo&) = delete;
  ~PexDebugFunctionInfo() = default;

  static PexDebugFunctionInfo* read(allocators::ChainedPool* alloc, PexReader& rdr);
  void write(PexWriter& wtr) const;

private:
  friend IntrusiveLinkedList<PexDebugFunctionInfo>;
  PexDebugFunctionInfo* next { nullptr };
};

}}
