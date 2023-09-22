#pragma once

#include <cstdint>

#include <common/IntrusiveLinkedList.h>

namespace caprica { namespace pex {

struct PexLabel final {
  size_t targetIdx { (size_t)-1 };

  explicit PexLabel() = default;
  PexLabel(const PexLabel&) = delete;
  ~PexLabel() = default;

private:
  friend IntrusiveLinkedList<PexLabel>;
  PexLabel* next { nullptr };
};

}}
