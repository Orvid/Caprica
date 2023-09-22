#pragma once

#include <cstdint>

#include <common/IntrusiveLinkedList.h>

namespace caprica { namespace pex {

struct PexString {
  size_t index { (size_t)-1 };

  explicit PexString() = default;
  PexString(const PexString&) = default;
  PexString(PexString&& other) = default;
  PexString& operator=(const PexString&) = default;
  PexString& operator=(PexString&&) = default;
  ~PexString() = default;

  bool operator<(const PexString& other) const noexcept { return index < other.index; }

  bool operator==(const PexString& other) const noexcept { return index == other.index; }

  bool operator!=(const PexString& other) const noexcept { return index != other.index; }

  bool valid() const noexcept { return index != -1; }
};

struct IntrusivePexString final : public PexString {
  IntrusivePexString(const PexString& str) : PexString(str) { }
  IntrusivePexString(PexString&& str) : PexString(str) { }

private:
  friend IntrusiveLinkedList<IntrusivePexString>;
  IntrusivePexString* next { nullptr };
};

}}
