#pragma once

#include <cstdint>

namespace caprica { namespace pex {

struct PexLabel final
{
  size_t targetIdx{ (size_t)-1 };

  PexLabel() = default;
  ~PexLabel() = default;
};

}}