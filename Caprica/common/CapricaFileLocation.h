#pragma once

#include <cstdint>

namespace caprica {

struct CapricaFileLocation final {
  uint32_t startOffset { 0 };
  uint32_t endOffset { 0 };

  explicit CapricaFileLocation() noexcept = default;
  explicit CapricaFileLocation(uint32_t start, uint32_t end) noexcept : startOffset(start), endOffset(end) { }
  ~CapricaFileLocation() noexcept = default;

  CapricaFileLocation throughStartOf(CapricaFileLocation endLocation) const noexcept {
    return CapricaFileLocation(startOffset, endLocation.startOffset);
  }

  CapricaFileLocation throughEndOf(CapricaFileLocation endLocation) const noexcept {
    return CapricaFileLocation(startOffset, endLocation.endOffset);
  }

  CapricaFileLocation startPlus(int add) const noexcept { return CapricaFileLocation(startOffset + add, endOffset); }
};

}
