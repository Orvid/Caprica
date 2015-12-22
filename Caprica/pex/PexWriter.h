#pragma once

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <sstream>

#include <pex/PexString.h>
#include <pex/PexUserFlags.h>
#include <pex/PexValue.h>

namespace caprica { namespace pex {

struct PexWriter final
{
  PexWriter(std::ostream& dest) : strm(dest) {

  }
  ~PexWriter() = default;

  template<typename T>
  void boundWrite(size_t val) {
    assert(val <= std::numeric_limits<T>::max());
    write((T)val);
  }

  template<typename T>
  void write(T val) {
    static_assert(false, "Invalid type passed to write!");
  }
  template<>
  void write(uint8_t val) {
    strm.write((char*)&val, sizeof(val));
  }
  template<>
  void write(uint16_t val) {
    strm.write((char*)&val, sizeof(val));
  }
  template<>
  void write(uint32_t val) {
    strm.write((char*)&val, sizeof(val));
  }
  template<>
  void write(time_t val) {
    static_assert(sizeof(time_t) == 8, "time_t is not 64 bits");
    strm.write((char*)&val, sizeof(val));
  }
  template<>
  void write(PexString val) {
    assert(val.index != -1);
    boundWrite<uint16_t>(val.index);
  }
  template<>
  void write(PexUserFlags val) {
    boundWrite<uint32_t>(val.data);
  }
  template<>
  void write(PexValue val) {

  }
  template<>
  void write(std::string val) {
    boundWrite<uint16_t>(val.size());
    strm.write(val.c_str(), val.size());
  }

  // This is intended specifically for use when
  // writing a PexObject, which needs to know
  // the full length of its data before writing
  // the actual data.
  void writeStream(std::stringstream& s) {
    auto str = s.str();
    strm.write(str.c_str(), str.size());
  }

private:
  std::ostream& strm;
};

}}