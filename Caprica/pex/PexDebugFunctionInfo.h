#pragma once

#include <ctime>
#include <vector>

#include <pex/PexString.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

enum class PexDebugFunctionType
{
  Normal = 0,
  Getter = 1,
  Setter = 2,
};

struct PexDebugFunctionInfo final
{
  PexString objectName{ };
  PexString stateName{ };
  PexString functionName{ };
  PexDebugFunctionType functionType{ PexDebugFunctionType::Normal };
  std::vector<uint16_t> instructionLineMap{ };

  PexDebugFunctionInfo() = default;
  ~PexDebugFunctionInfo() = default;

  void write(PexWriter& wtr) const;
};

}}
