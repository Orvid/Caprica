#pragma once

#include <cstdint>

#include <pex/PexLabel.h>
#include <pex/PexLocalVariable.h>
#include <pex/PexString.h>

namespace caprica { namespace pex {

enum class PexValueType : uint8_t
{
  None = 0,
  Identifier = 1,
  String = 2,
  Integer = 3,
  Float = 4,
  Bool = 5,

  Label = 20,
};

struct PexValue final
{
  PexValueType type{ PexValueType::None };
  union
  {
    PexString s;
    int32_t i;
    float f;
    bool b;
    PexLabel* l;
  };

  struct Identifier
  {
    PexString name;

    Identifier(PexString str) : name(str) { }
    Identifier(PexLocalVariable* var) : name(var->name) { }
    ~Identifier() = default;
  };

  struct Integer
  {
    int32_t i;

    Integer(int32_t val) : i(val) { }
    ~Integer() = default;
  };

  struct Bool
  {
    bool b;

    Bool(bool val) : b(val) { }
    ~Bool() = default;
  };

  struct None { };

  PexValue() { };
  PexValue(const PexValue&) = default;
  PexValue(PexLabel* lab) : type(PexValueType::Label), l(lab) { }
  PexValue(PexLocalVariable* var) : type(PexValueType::Identifier), s(var->name) { }
  PexValue(const Identifier& id) : type(PexValueType::Identifier), s(id.name) { }
  PexValue(const Integer& val) : type(PexValueType::Integer), i(val.i) { }
  PexValue(const Bool& val) : type(PexValueType::Bool), b(val.b) { }
  PexValue(const None& val) : type(PexValueType::None) { }
  ~PexValue() = default;
};

}}
