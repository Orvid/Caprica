#pragma once

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

#include <common/FSUtils.h>

namespace caprica {

struct CapricaBinaryWriter
{
  explicit CapricaBinaryWriter() = default;
  CapricaBinaryWriter(const CapricaBinaryWriter&) = delete;
  ~CapricaBinaryWriter() = default;

  std::string getOutputBuffer() const {
    return strm;
  }

  void writeToFile(const std::string& filename) const {
    FSUtils::async_write(filename, getOutputBuffer());
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
    strm.append((char*)&val, sizeof(val));
  }

  template<>
  void write(uint8_t val) {
    strm.append((char*)&val, sizeof(val));
  }

  template<>
  void write(int16_t val) {
    strm.append((char*)&val, sizeof(val));
  }

  template<>
  void write(uint16_t val) {
    strm.append((char*)&val, sizeof(val));
  }

  template<>
  void write(int32_t val) {
    strm.append((char*)&val, sizeof(val));
  }

  template<>
  void write(uint32_t val) {
    strm.append((char*)&val, sizeof(val));
  }

  template<>
  void write(float val) {
    strm.append((char*)&val, sizeof(val));
  }

  template<>
  void write(time_t val) {
    static_assert(sizeof(time_t) == 8, "time_t is not 64 bits");
    strm.append((char*)&val, sizeof(val));
  }

  template<>
  void write(const std::string& val) {
    boundWrite<uint16_t>(val.size());
    if (val.size())
      strm.append(val.c_str(), val.size());
  }

protected:
  std::string strm{ };
};

}
