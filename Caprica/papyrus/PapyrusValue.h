#pragma once

#include <cstdint>

#include <boost/utility/string_ref.hpp>

#include <common/CapricaFileLocation.h>

#include <papyrus/PapyrusType.h>

#include <pex/PexFile.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus {

enum class PapyrusValueType
{
  Invalid = -1,

  None = 0,
  String,
  Integer,
  Float,
  Bool,
};

struct PapyrusValue final
{
  PapyrusValueType type{ PapyrusValueType::None };
  CapricaFileLocation location;
  boost::string_ref s;
  union
  {
    int32_t i;
    float f;
    bool b;
  };

  // This is intended purely for initializing values that may not
  // be filled to defaults, and doesn't track a location.
  struct Default final { };

  PapyrusValue() = delete;
  explicit PapyrusValue(CapricaFileLocation loc) : location(loc) { }
  PapyrusValue(const Default& other) : type(PapyrusValueType::Invalid), location(0) { }
  PapyrusValue(const PapyrusValue& other) = default;
  PapyrusValue(PapyrusValue&& other) = default;
  PapyrusValue& operator =(const PapyrusValue&) = default;
  PapyrusValue& operator =(PapyrusValue&&) = default;
  ~PapyrusValue() = default;

  static PapyrusValue None(CapricaFileLocation loc) {
    auto val = PapyrusValue(loc);
    val.type = PapyrusValueType::None;
    return val;
  }

  static PapyrusValue Bool(CapricaFileLocation loc, bool b) {
    auto val = PapyrusValue(loc);
    val.type = PapyrusValueType::Bool;
    val.b = b;
    return val;
  }

  static PapyrusValue String(CapricaFileLocation loc, boost::string_ref s) {
    auto val = PapyrusValue(loc);
    val.type = PapyrusValueType::String;
    val.s = s;
    return val;
  }

  static PapyrusValue Integer(CapricaFileLocation loc, int32_t i) {
    auto val = PapyrusValue(loc);
    val.type = PapyrusValueType::Integer;
    val.i = i;
    return val;
  }

  static PapyrusValue Float(CapricaFileLocation loc, float f) {
    auto val = PapyrusValue(loc);
    val.type = PapyrusValueType::Float;
    val.f = f;
    return val;
  }

  pex::PexValue buildPex(pex::PexFile* file) const {
    pex::PexValue pe;
    switch (type) {
      // Invalid always gets written as None.
      case PapyrusValueType::Invalid:
      case PapyrusValueType::None:
        pe.type = pex::PexValueType::None;
        return pe;
      case PapyrusValueType::String:
        pe.type = pex::PexValueType::String;
        pe.s = file->getString(s);
        return pe;
      case PapyrusValueType::Integer:
        pe.type = pex::PexValueType::Integer;
        pe.i = i;
        return pe;
      case PapyrusValueType::Float:
        pe.type = pex::PexValueType::Float;
        pe.f = f;
        return pe;
      case PapyrusValueType::Bool:
        pe.type = pex::PexValueType::Bool;
        pe.b = b;
        return pe;
    }
    CapricaReportingContext::logicalFatal("Unknown PapyrusValueType!");
  }

  PapyrusType getPapyrusType() const {
    switch (type) {
      case PapyrusValueType::None:
        return PapyrusType::None(location);
      case PapyrusValueType::String:
        return PapyrusType::String(location);
      case PapyrusValueType::Integer:
        return PapyrusType::Int(location);
      case PapyrusValueType::Float:
        return PapyrusType::Float(location);
      case PapyrusValueType::Bool:
        return PapyrusType::Bool(location);

      case PapyrusValueType::Invalid:
        break;
    }
    CapricaReportingContext::logicalFatal("Unknown PapyrusValueType!");
  }
};

}}
