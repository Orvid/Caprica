#pragma once

#include <cstdint>

namespace caprica { namespace pex {

struct PexString final
{
  size_t index{ (size_t)-1 };

  bool operator <(const PexString& other) const {
    return index < other.index;
  }

  bool valid() const {
    return index != -1;
  }
};

}}
