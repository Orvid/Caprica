#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

#include <common/CapricaError.h>

namespace caprica {

struct CapricaBinaryReader
{
  explicit CapricaBinaryReader(const std::string& file) : strm(file, std::ifstream::binary) {
    strm.exceptions(std::ifstream::badbit | std::ifstream::failbit);
  }
  ~CapricaBinaryReader() = default;

  bool eof() {
    strm.peek();
    return strm.eof();
  }

  template<typename T>
  T read() {
    static_assert(false, "Invalid type passed to read!");
  }

  template<>
  int8_t read() {
    int8_t val;
    strm.read((char*)&val, sizeof(val));
    return val;
  }

  template<>
  uint8_t read() {
    uint8_t val = 0;
    strm.read((char*)&val, sizeof(val));
    return val;
  }

  template<>
  int16_t read() {
    int16_t val;
    strm.read((char*)&val, sizeof(val));
    return val;
  }

  template<>
  uint16_t read() {
    uint16_t val;
    strm.read((char*)&val, sizeof(val));
    return val;
  }

  template<>
  int32_t read() {
    int32_t val;
    strm.read((char*)&val, sizeof(val));
    return val;
  }

  template<>
  uint32_t read() {
    uint32_t val;
    strm.read((char*)&val, sizeof(val));
    return val;
  }

  template<>
  float read() {
    float val;
    strm.read((char*)&val, sizeof(val));
    return val;
  }

  template<>
  time_t read() {
    static_assert(sizeof(time_t) == 8, "time_t is not 64 bits");
    time_t val;
    strm.read((char*)&val, sizeof(val));
    return val;
  }

  template<>
  std::string read() {
    auto len = read<uint16_t>();
    std::unique_ptr<char[]> buf(new char[len]);
    strm.read(buf.get(), len);
    return std::string(buf.get(), buf.get() + len);
  }

protected:
  std::ifstream strm;
};

}
