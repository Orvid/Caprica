#pragma once

#include <cassert>
#include <cstdint>
#include <limits>

#include <pex/PexString.h>

namespace caprica { namespace pex {

template<typename T>
struct FixedPexStringMap final
{
  void insert(PexString str, T* val) {
    assert(str.index < MaxTotalEntries);
    assert(entries[str.index] == nullptr);
    if (maxUsedEntry < str.index)
      maxUsedEntry = str.index;
    entries[str.index] = val;
  }

  T* findOrCreate(PexString str) {
    T* ret;
    if (!tryFind(str, ret)) {
      ret = new T();
      insert(str, ret);
    }
    return ret;
  }

  bool tryFind(PexString str, T*& dest) {
    if (str.index <= maxUsedEntry) {
      dest = entries[str.index];
      return dest != nullptr;
    }
    return false;
  }

  void reset() {
    const auto resetSize = sizeof(T*) * (maxUsedEntry + 1);
    maxUsedEntry = 0;
    memset(entries, 0, resetSize);
  }
private:
  static constexpr size_t MaxTotalEntries = std::numeric_limits<uint16_t>::max();
  size_t maxUsedEntry{ 0 };
  T* entries[MaxTotalEntries];
};

}}
