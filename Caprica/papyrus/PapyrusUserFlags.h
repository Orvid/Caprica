#pragma once

#include <string>
#include <type_traits>
#include <vector>

#include <boost/algorithm/string/case_conv.hpp>

#include <pex/PexFile.h>
#include <pex/PexUserFlags.h>

namespace caprica { namespace papyrus {

struct PapyrusUserFlags final
{
  size_t data{ 0 };

  // These properties are for the sanity of the parser.
  // Not all of them are valid on all declarations.

  bool isAuto{ false };
  bool isAutoReadOnly{ false };
  bool isBetaOnly{ false };
  bool isConst{ false };
  bool isDebugOnly{ false };
  bool isGlobal{ false };
  bool isNative{ false };

  explicit PapyrusUserFlags() = default;
  PapyrusUserFlags(const PapyrusUserFlags&) = default;
  ~PapyrusUserFlags() = default;

  pex::PexUserFlags buildPex(pex::PexFile* file, CapricaUserFlagsDefinition::ValidLocations limitLocations = CapricaUserFlagsDefinition::ValidLocations::AllLocations) const {
    pex::PexUserFlags pexFlags;
    size_t f = 1;
    for (uint8_t i = 0; i < sizeof(size_t) * 8; i++) {
      if ((data & f) == f) {
        auto uf = CapricaConfig::userFlagsDefinition.getFlag(i);
        if ((uf.validLocations & limitLocations) != CapricaUserFlagsDefinition::ValidLocations::None) {
          auto lowerName = uf.name;
          boost::algorithm::to_lower(lowerName);
          pexFlags |= file->getUserFlag(file->getString(lowerName), uf.bitIndex);
        }
      }
      f <<= 1;
    }
    return pexFlags;
  }

  PapyrusUserFlags& operator |=(const PapyrusUserFlags& b) {
    data |= b.data;
    return *this;
  }

};

}}
