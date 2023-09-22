#include <papyrus/PapyrusUserFlags.h>

namespace caprica { namespace papyrus {

pex::PexUserFlags PapyrusUserFlags::buildPex(pex::PexFile* file,
                                             CapricaUserFlagsDefinition::ValidLocations limitLocations) const {
  pex::PexUserFlags pexFlags;
  size_t f = 1;
  for (uint8_t i = 0; i < sizeof(size_t) * 8; i++) {
    if ((data & f) == f) {
      auto uf = conf::Papyrus::userFlagsDefinition.getFlag(i);
      if ((uf.validLocations & limitLocations) != CapricaUserFlagsDefinition::ValidLocations::None) {
        auto lowerName = uf.name;
        identifierToLower(lowerName);
        pexFlags |= file->getUserFlag(file->getString(lowerName), uf.bitIndex);
      }
    }
    f <<= 1;
  }
  return pexFlags;
}

}}
