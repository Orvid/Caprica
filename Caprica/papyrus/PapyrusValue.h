#pragma once

#include <cstdint>

#include <common/CapricaFileLocation.h>

#include <pex/PexFile.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus {

enum class PapyrusValueType
{
  None,
  String,
  Integer,
  Float,
  Bool,
};

struct PapyrusValue final
{
  PapyrusValueType type{ PapyrusValueType::None };
  CapricaFileLocation location;
  std::string s;
  union
  {
    int32_t i;
    float f;
    bool b;
  };

  // This is intended purely for initializing values that may not
  // be filled to defaults, and doesn't track a location.
  struct Default final { };

  struct None final
  {
    CapricaFileLocation location;

    None(const CapricaFileLocation& loc) : location(loc) { }
    ~None() = default;
  };

  struct Integer final
  {
    int32_t i;
    CapricaFileLocation location;

    Integer(const CapricaFileLocation& loc, int32_t val) : location(loc), i(val) { }
    ~Integer() = default;
  };

  PapyrusValue(const CapricaFileLocation& loc) : location(loc) { }
  PapyrusValue(const Default& other) : type(PapyrusValueType::None), location("", 0, 0) { }
  PapyrusValue(const None& other) : type(PapyrusValueType::None), location(other.location) { }
  PapyrusValue(const Integer& other) : type(PapyrusValueType::Integer), location(other.location), i(other.i) { }
  PapyrusValue(const PapyrusValue& other) = default;
  ~PapyrusValue() = default;

  pex::PexValue buildPex(pex::PexFile* file) const {
    pex::PexValue pe;
    switch (type) {
      case PapyrusValueType::None:
        pe.type = pex::PexValueType::None;
        break;
      case PapyrusValueType::String:
        pe.type = pex::PexValueType::String;
        pe.s = file->getString(s);
        break;
      case PapyrusValueType::Integer:
        pe.type = pex::PexValueType::Integer;
        pe.i = i;
        break;
      case PapyrusValueType::Float:
        pe.type = pex::PexValueType::Float;
        pe.f = f;
        break;
      case PapyrusValueType::Bool:
        pe.type = pex::PexValueType::Bool;
        pe.b = b;
        break;
      default:
        assert(0);
        break;
    }
    return pe;
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
      default:
        CapricaError::logicalFatal("Unknown PapyrusValueType!");
    }
  }
};

}}
