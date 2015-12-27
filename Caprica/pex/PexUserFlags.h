#pragma once

#include <cstdint>

namespace caprica { namespace pex {

struct PexUserFlags final
{
  size_t data{ 0 };

  PexUserFlags() = default;
  ~PexUserFlags() = default;

  PexUserFlags& operator |=(const PexUserFlags& b) {
    data |= b.data;
    return *this;
  }
};

}}
