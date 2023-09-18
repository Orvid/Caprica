#pragma once

#include <cassert>
#include <cstdint>
#include <limits>

#include <common/CapricaBinaryWriter.h>
#include <common/CapricaReportingContext.h>

#include <pex/PexString.h>
#include <pex/PexUserFlags.h>
#include <pex/PexValue.h>
#include "common/GameID.h"

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
  void write(GameID val) {
    write<uint16_t>(static_cast<uint16_t>(val));
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
        write<PexString>(val.val.s);
        return;
      case PexValueType::Integer:
        write<uint32_t>((uint32_t)val.val.i);
        return;
      case PexValueType::Float:
        write<float>(val.val.f);
        return;
      case PexValueType::Bool:
        write<uint8_t>(val.val.b ? 0x01 : 0x00);
        return;

      case PexValueType::Label:
      case PexValueType::TemporaryVar:
      case PexValueType::Invalid:
        break;
    }
    CapricaReportingContext::logicalFatal("Unknown PexValueType!");
  }

  void beginObject() {
    objectLength = strm.make<uint32_t>();
    objectStartSize = strm.totalAllocatedBytes();
  }

  void endObject() {
    assert(strm.totalAllocatedBytes() - objectStartSize <= std::numeric_limits<uint32_t>::max());
    *objectLength = (uint32_t)(strm.totalAllocatedBytes() - objectStartSize);
    objectLength = nullptr;
  }

private:
  uint32_t* objectLength{ nullptr };
  size_t objectStartSize{ 0 };
};

}}
