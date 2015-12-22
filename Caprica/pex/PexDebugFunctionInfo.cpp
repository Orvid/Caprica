#include <pex/PexDebugFunctionInfo.h>

namespace caprica { namespace pex {

void PexDebugFunctionInfo::write(PexWriter& wtr) const {
  wtr.write<PexString>(objectName);
  wtr.write<PexString>(stateName);
  wtr.write<PexString>(functionName);
  wtr.write<uint8_t>((uint8_t)functionType);
  wtr.boundWrite<uint16_t>(instructionLineMap.size());
  for (auto l : instructionLineMap)
    wtr.write<uint16_t>(l);
}

}}