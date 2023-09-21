#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <string_view>

#include <common/ByteSwap.h>
#include <common/FSUtils.h>
#include <common/allocators/ChainedPool.h>

namespace caprica {

struct CapricaBinaryWriter
{
  Endianness endianness{ Endianness::Little };
  explicit CapricaBinaryWriter() = default;
  CapricaBinaryWriter(const CapricaBinaryWriter&) = delete;
  ~CapricaBinaryWriter() = default;

  template<typename F>
  void applyToBuffers(F&& func) {
    for (auto beg = strm.begin(), end = strm.end(); beg != end; ++beg) {
      func(beg.data(), beg.size());
    }
  }

  template<typename T>
  void boundWrite(size_t val) {
    assert(val <= std::numeric_limits<T>::max());
    write<T>((T)val);
  }

  template<typename T>
  void write(T val) {
    static_assert(std::is_same_v<T, void>, "Invalid type passed to write!");
  }

  template<>
  void write(int8_t val) {
    strm.make<int8_t>(val);
  }

  template<>
  void write(uint8_t val) {
    strm.make<uint8_t>(val);
  }

  template<>
  void write(int16_t val) {
    strm.make<int16_t>(endianness == Endianness::Little ? val : byteswap(val));
  }

  template<>
  void write(uint16_t val) {
    strm.make<uint16_t>(endianness == Endianness::Little ? val : byteswap(val));
  }

  template<>
  void write(int32_t val) {
    strm.make<int32_t>(endianness == Endianness::Little ? val : byteswap(val));
  }

  template<>
  void write(uint32_t val) {
    strm.make<uint32_t>(endianness == Endianness::Little ? val : byteswap(val));
  }

  template<>
  void write(float val) {
    strm.make<float>(endianness == Endianness::Little ? val : byteswap_float(val));
  }

  template<>
  void write(time_t val) {
    static_assert(sizeof(time_t) == 8, "time_t is not 64 bits");
    strm.make<time_t>(endianness == Endianness::Little ? val : byteswap(val));
  }

  template<>
  void write(std::string_view val) {
    boundWrite<uint16_t>(val.size());
    if (val.size())
      append(val.data(), val.size());
  }

  template<>
  void write(identifier_ref val) {
    boundWrite<uint16_t>(val.size());
    if (val.size())
      append(val.data(), val.size());
  }

protected:
  allocators::ChainedPool strm{ 1024 * 4 };

  void append(const char* __restrict a, size_t size) {
    // ChainedPool will re-order large allocations, so disallow them.
    assert(size < 4096);
    memcpy(strm.allocate(size), a, size);
  }
};

}
