#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

#include <CapricaError.h>

#include <pex/PexString.h>
#include <pex/PexUserFlags.h>
#include <pex/PexValue.h>

namespace caprica { namespace pex {

struct PexReader final
{
  PexReader(const std::string& file) : strm(file, std::ifstream::binary) { }
  ~PexReader() = default;

  template<typename T>
  T read() {
    static_assert(false, "Invalid type passed to write!");
  }

  template<>
  uint8_t read() {
    uint8_t val;
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
  uint32_t read() {
    uint32_t val;
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
  PexString read() {
    PexString val;
    val.index = 0;
    strm.read((char*)&val.index, sizeof(uint16_t));
    return val;
  }

  template<>
  PexUserFlags read() {
    PexUserFlags val;
    val.data = 0;
    strm.read((char*)&val.data, sizeof(uint32_t));
    return val;
  }

  template<>
  PexValue read() {
    PexValue val;
    val.type = (PexValueType)read<uint8_t>();
    switch (val.type) {
      case PexValueType::None:
        break;
      case PexValueType::Identifier:
      case PexValueType::String:
        val.s = read<PexString>();
        break;
      case PexValueType::Integer:
        val.i = (int32_t)read<uint32_t>();
        break;
      case PexValueType::Float:
      {
        float f;
        strm.read((char*)&f, sizeof(float));
        val.f = f;
        break;
      }
      case PexValueType::Bool:
        val.b = read<uint8_t>() ? true : false;
        break;
      default:
        CapricaError::logicalFatal("Unknown PexValueType!");
    }
    return val;
  }

  template<>
  std::string read() {
    auto len = read<uint16_t>();
    std::unique_ptr<char[]> buf(new char[len]);
    strm.read(buf.get(), len);
    return std::string(buf.get(), buf.get() + len);
  }

private:
  std::ifstream strm;
};

}}