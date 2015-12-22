#pragma once

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

namespace caprica { namespace pex {

struct PexWriter final
{
  PexWriter(std::string filename) : strm(filename) {

  }
  ~PexWriter() = default;

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
  void write(std::string val) {
    assert(val.size() <= std::numeric_limits<uint16_t>::max());
    write<uint16_t>((uint16_t)val.size());
    strm.write(val.c_str(), val.size());
  }
private:
  std::ofstream strm;
};

}}