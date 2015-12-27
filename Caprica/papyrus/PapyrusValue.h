#pragma once

#include <cstdint>

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
  std::string s;
  union
  {
    int32_t i;
    float f;
    bool b;
  };

  PapyrusValue() = default;
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
        return PapyrusType::None();
      case PapyrusValueType::String:
        return PapyrusType::String();
      case PapyrusValueType::Integer:
        return PapyrusType::Int();
      case PapyrusValueType::Float:
        return PapyrusType::Float();
      case PapyrusValueType::Bool:
        return PapyrusType::Bool();
      default:
        throw std::runtime_error("Unknown PapyrusValueType!");
    }
  }
};

}}
