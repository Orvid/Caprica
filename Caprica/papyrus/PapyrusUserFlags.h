#pragma once

#include <common/CapricaConfig.h>
#include <common/CapricaUserFlagsDefinition.h>

#include <pex/PexFile.h>
#include <pex/PexUserFlags.h>

namespace caprica { namespace papyrus {

struct PapyrusUserFlags final {
  size_t data { 0 };

  // These properties are for the sanity of the parser.
  // Not all of them are valid on all declarations.

  bool isAuto { false };
  bool isAutoReadOnly { false };
  bool isBetaOnly { false };
  bool isConst { false };
  bool isDebugOnly { false };
  bool isGlobal { false };
  bool isNative { false };

  explicit PapyrusUserFlags() = default;
  PapyrusUserFlags(const PapyrusUserFlags&) = default;
  ~PapyrusUserFlags() = default;

  pex::PexUserFlags buildPex(pex::PexFile* file,
                             CapricaUserFlagsDefinition::ValidLocations limitLocations =
                                 CapricaUserFlagsDefinition::ValidLocations::AllLocations) const;

  PapyrusUserFlags& operator|=(const PapyrusUserFlags& b) {
    data |= b.data;
    return *this;
  }
};

}}
