#pragma once

#include <cstdint>

namespace caprica { namespace pex {

struct PexString final
{
  size_t index{ (size_t)-1 };

  explicit PexString() = default;
  PexString(const PexString&) = default;
  PexString(PexString&& other) = default;
  PexString& operator =(const PexString&) = default;
  PexString& operator =(PexString&&) = default;
  ~PexString() = default;

  bool operator <(const PexString& other) const noexcept {
    return index < other.index;
  }

  bool operator ==(const PexString& other) const noexcept {
    return index == other.index;
  }

  bool operator !=(const PexString& other) const noexcept {
    return index != other.index;
  }

  bool valid() const noexcept {
    return index != -1;
  }
};

}}

#include <xstddef>
namespace std {
template <>
struct hash<caprica::pex::PexString>
{
  std::size_t operator()(const caprica::pex::PexString& k) const {
    return std::hash<size_t>()(k.index);
  }
};
}
