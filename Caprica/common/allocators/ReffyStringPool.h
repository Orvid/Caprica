#pragma once

#include <stdint.h>
#include <limits>

#include <boost/utility/string_ref.hpp>

#include <common/allocators/ChainedPool.h>

namespace caprica { namespace allocators {

struct ReffyStringPool final
{
  static constexpr size_t MaxCapacity = std::numeric_limits<uint16_t>::max();

  ReffyStringPool() = default;

  size_t lookup(boost::string_ref str);
  boost::string_ref byIndex(size_t v) const;
  void push_back(boost::string_ref str);
  void reset();
  size_t size() const { return count; };

private:
  struct StringHeader final
  {
    uint16_t length{ 0 };
    const char* data{ nullptr };
  };
  struct HashEntry final
  {
    uint32_t generationNum{ 0 };
    uint16_t stringIndex{ 0 };
    uint16_t upperHash{ 0 };
  };

  static constexpr size_t mask = MaxCapacity;
  uint32_t generationNumber{ 1 };
  size_t count{ 0 };
  ChainedPool alloc{ 1024 * 4 };
  StringHeader* strings[MaxCapacity]{ };
  HashEntry hashtable[MaxCapacity]{ };

  HashEntry* find(boost::string_ref str, size_t hash);
  size_t push_back_with_hash(boost::string_ref str, size_t hash, HashEntry* entry);
  static size_t hash(boost::string_ref str);
};

}}
