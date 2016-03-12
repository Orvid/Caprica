#pragma once

#include <cstdint>

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

    explicit None(const CapricaFileLocation& loc) : location(loc) { }
    None(const None&) = delete;
    ~None() = default;
  };
  struct Integer final
  {
    int32_t i;
    CapricaFileLocation location;

    explicit Integer(const CapricaFileLocation& loc, int32_t val) : location(loc), i(val) { }
    Integer(const Integer&) = delete;
    ~Integer() = default;
  };
  struct Float final
  {
    float f;
    CapricaFileLocation location;

    explicit Float(const CapricaFileLocation& loc, float val) : location(loc), f(val) { }
    Float(const Float&) = delete;
    ~Float() = default;
  };

  PapyrusValue() = delete;
  explicit PapyrusValue(const CapricaFileLocation& loc) : location(loc) { }
  PapyrusValue(const Default& other) : type(PapyrusValueType::Invalid), location("", 0, 0) { }
  PapyrusValue(const None& other) : type(PapyrusValueType::None), location(other.location) { }
  PapyrusValue(const Integer& other) : type(PapyrusValueType::Integer), location(other.location), i(other.i) { }
  PapyrusValue(const Float& other) : type(PapyrusValueType::Float), location(other.location), f(other.f) { }
  PapyrusValue(const PapyrusValue& other) = default;
  PapyrusValue(PapyrusValue&& other) = default;
  PapyrusValue& operator =(const PapyrusValue&) = default;
  PapyrusValue& operator =(PapyrusValue&&) = default;
  ~PapyrusValue() = default;

  pex::PexValue buildPex(pex::PexFile* file) const {
    pex::PexValue pe;
    switch (type) {
      // Invalid always gets written as None.
      case PapyrusValueType::Invalid:
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
