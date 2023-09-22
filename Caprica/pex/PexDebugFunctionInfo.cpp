#include <pex/PexDebugFunctionInfo.h>

namespace caprica { namespace pex {

PexDebugFunctionInfo* PexDebugFunctionInfo::read(allocators::ChainedPool* alloc, PexReader& rdr) {
  auto fi = alloc->make<PexDebugFunctionInfo>();
  fi->objectName = rdr.read<PexString>();
  fi->stateName = rdr.read<PexString>();
  fi->functionName = rdr.read<PexString>();
  fi->functionType = (PexDebugFunctionType)rdr.read<uint8_t>();
  auto lnSize = rdr.read<uint16_t>();
  fi->instructionLineMap.reserve(lnSize);
  for (size_t i = 0; i < lnSize; i++)
    fi->instructionLineMap.push_back(rdr.read<uint16_t>());
  return fi;
}

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
