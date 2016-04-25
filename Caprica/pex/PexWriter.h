#pragma once

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <sstream>

#include <common/CapricaBinaryWriter.h>
#include <common/CapricaReportingContext.h>

#include <pex/PexString.h>
#include <pex/PexUserFlags.h>
#include <pex/PexValue.h>

namespace caprica { namespace pex {

struct PexWriter final : public CapricaBinaryWriter
{
  explicit PexWriter() = default;
  PexWriter(const PexWriter&) = delete;
  ~PexWriter() = default;

  template<typename T>
  void write(T val) {
    CapricaBinaryWriter::write<T>(std::forward<T>(val));
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
    write<uint8_t>((uint8_t)val.type);
    switch (val.type) {
      case PexValueType::None:
        return;
      case PexValueType::Identifier:
      case PexValueType::String:
        write<PexString>(val.s);
        return;
      case PexValueType::Integer:
        write<uint32_t>((uint32_t)val.i);
        return;
      case PexValueType::Float:
        write<float>(val.f);
        return;
      case PexValueType::Bool:
        write<uint8_t>(val.b ? 0x01 : 0x00);
        return;

      case PexValueType::Label:
      case PexValueType::TemporaryVar:
      case PexValueType::Invalid:
        break;
    }
    CapricaReportingContext::logicalFatal("Unknown PexValueType!");
  }

  // This is intended specifically for use when
  // writing a PexObject, which needs to know
  // the full length of its data before writing
  // the actual data.
  void writeStream(const std::string& str) {
    strm.write(str.c_str(), str.size());
  }
};

}}
