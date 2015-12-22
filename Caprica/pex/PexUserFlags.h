#pragma once

#include <cstdint>

namespace caprica { namespace pex {

struct PexUserFlag final
{
  size_t data{ 0 };
};

struct PexUserFlags final
{
  size_t data{ 0 };

  PexUserFlags() = default;
  PexUserFlags(const PexUserFlag& flag) : data(flag.data) { }
  ~PexUserFlags() = default;
};

}}
