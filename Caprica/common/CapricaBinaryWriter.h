#pragma once

#include <cassert>
#include <cstdint>
#include <limits>

#include <boost/utility/string_ref.hpp>

#include <common/FSUtils.h>
#include <common/allocators/ChainedPool.h>

namespace caprica {

struct CapricaBinaryWriter
{
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
    static_assert(false, "Invalid type passed to write!");
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
    strm.make<int16_t>(val);
  }

  template<>
  void write(uint16_t val) {
    strm.make<uint16_t>(val);
  }

  template<>
  void write(int32_t val) {
    strm.make<int32_t>(val);
  }

  template<>
  void write(uint32_t val) {
    strm.make<uint32_t>(val);
  }

  template<>
  void write(float val) {
    strm.make<float>(val);
  }

  template<>
  void write(time_t val) {
    static_assert(sizeof(time_t) == 8, "time_t is not 64 bits");
    strm.make<time_t>(val);
  }

  template<>
  void write(boost::string_ref val) {
    boundWrite<uint16_t>(val.size());
    if (val.size())
      append(val.data(), val.size());
  }

protected:
  allocators::ChainedPool strm{ 1024 * 4 };

  void append(const char* __restrict a, size_t size) {
    memcpy(strm.allocate(size), a, size);
  }
};

}
