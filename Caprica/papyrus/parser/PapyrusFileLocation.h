#pragma once

#include <cassert>
#include <cstdint>
#include <limits>

namespace caprica { namespace papyrus { namespace parser {

struct PapyrusFileLocation
{
  size_t line{ 0 };

  PapyrusFileLocation() = default;
  ~PapyrusFileLocation() = default;

  uint16_t buildPex() const {
    assert(line <= std::numeric_limits<uint16_t>::max());
    return (uint16_t)line;
  }
};

}}}
