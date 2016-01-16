#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

#include <common/CapricaBinaryReader.h>
#include <common/CapricaError.h>

#include <pex/PexString.h>
#include <pex/PexUserFlags.h>
#include <pex/PexValue.h>

namespace caprica { namespace pex {

struct PexReader final : public CapricaBinaryReader
{
  PexReader(const std::string& file) : CapricaBinaryReader(file) { }
  ~PexReader() = default;

  template<typename T>
  T read() {
    return CapricaBinaryReader::read<T>();
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
        val.f = read<float>();
        break;
      case PexValueType::Bool:
        val.b = read<uint8_t>() ? true : false;
        break;
      default:
        CapricaError::logicalFatal("Unknown PexValueType!");
    }
    return val;
  }
};

}}
