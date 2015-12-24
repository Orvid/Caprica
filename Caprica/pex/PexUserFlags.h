#pragma once

#include <cstdint>

#include <papyrus/PapyrusUserFlags.h>

namespace caprica { namespace pex {

struct PexUserFlag final
{
  size_t data{ 0 };
};

struct PexUserFlags final
{
  size_t data{ 0 };

  PexUserFlags() = default;
  // TODO: Remove.
  PexUserFlags(papyrus::PapyrusUserFlags flags) { }
  PexUserFlags(const PexUserFlag& flag) : data(flag.data) { }
  ~PexUserFlags() = default;
};

}}
