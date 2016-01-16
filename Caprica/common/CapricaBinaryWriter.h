#pragma once

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

namespace caprica {

struct CapricaBinaryWriter
{
  CapricaBinaryWriter(std::ostream& dest) : strm(dest) { }
  ~CapricaBinaryWriter() = default;

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
    strm.write((char*)&val, sizeof(val));
  }

  template<>
  void write(uint8_t val) {
    strm.write((char*)&val, sizeof(val));
  }

  template<>
  void write(int16_t val) {
    strm.write((char*)&val, sizeof(val));
  }

  template<>
  void write(uint16_t val) {
    strm.write((char*)&val, sizeof(val));
  }

  template<>
  void write(int32_t val) {
    strm.write((char*)&val, sizeof(val));
  }

  template<>
  void write(uint32_t val) {
    strm.write((char*)&val, sizeof(val));
  }

  template<>
  void write(float val) {
    strm.write((char*)&val, sizeof(val));
  }

  template<>
  void write(time_t val) {
    static_assert(sizeof(time_t) == 8, "time_t is not 64 bits");
    strm.write((char*)&val, sizeof(val));
  }

  template<>
  void write(std::string val) {
    boundWrite<uint16_t>(val.size());
    if (val.size())
      strm.write(val.c_str(), val.size());
  }

protected:
  std::ostream& strm;
};

}
