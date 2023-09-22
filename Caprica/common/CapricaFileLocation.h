#pragma once

#include <cstdint>

namespace caprica {

struct CapricaFileLocation final {
  size_t fileOffset { 0 };

  explicit CapricaFileLocation() = default;
  explicit CapricaFileLocation(size_t offset) noexcept : fileOffset(offset) { }
  ~CapricaFileLocation() = default;
};

}
