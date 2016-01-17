#pragma once

#include <cassert>
#include <cstdint>

#include <pex/PexAsmWriter.h>
#include <pex/PexLabel.h>
#include <pex/PexLocalVariable.h>
#include <pex/PexString.h>

namespace caprica { namespace pex {

struct PexFile;

enum class PexValueType : uint8_t
{
  None = 0,
  Identifier = 1,
  String = 2,
  Integer = 3,
  Float = 4,
  Bool = 5,

  Label = 20,
  TemporaryVar = 21,
  Invalid = 50,
};

struct PexTemporaryVariableRef
{
  PexString type{ };
  PexLocalVariable* var{ nullptr };

  explicit PexTemporaryVariableRef(const PexString& tp) : type(tp) { }
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
    PexTemporaryVariableRef* tmpVar;
  };

  struct TemporaryVariable
  {
    PexTemporaryVariableRef* var;

    explicit TemporaryVariable(PexTemporaryVariableRef* v) : var(v) { }
    TemporaryVariable(const TemporaryVariable&) = default;
    ~TemporaryVariable() = default;
  };
  struct Identifier
  {
    PexString name;
    PexTemporaryVariableRef* tmpVar{ nullptr };

    Identifier() = delete;
    Identifier(PexString str) : name(str) { }
    Identifier(PexLocalVariable* var) : name(var->name) { }
    Identifier(PexTemporaryVariableRef* v) : tmpVar(v) { }
    Identifier(const TemporaryVariable& var) : tmpVar(var.var) { }
    Identifier(const Identifier&) = default;
    ~Identifier() = default;

    static Identifier fromVar(const PexValue& var) {
      if (var.type == PexValueType::TemporaryVar)
        return Identifier(var.tmpVar);
      assert(var.type == PexValueType::Identifier);
      return Identifier(var.s);
    }
  };
  struct Integer
  {
    int32_t i;

    explicit Integer(int32_t val) : i(val) { }
    Integer(const Integer&) = delete;
    ~Integer() = default;
  };
  struct Float
  {
    float f;

    explicit Float(float val) : f(val) { }
    Float(const Float&) = delete;
    ~Float() = default;
  };
  struct Bool
  {
    bool b;

    explicit Bool(bool val) : b(val) { }
    Bool(const Bool&) = delete;
    ~Bool() = default;
  };
  struct None { };
  struct Invalid { };

  explicit PexValue() { };
  PexValue(const PexValue&) = default;
  PexValue(PexLabel* lab) : type(PexValueType::Label), l(lab) { }
  PexValue(PexLocalVariable* var) : type(PexValueType::Identifier), s(var->name) { }
  PexValue(const TemporaryVariable& val) : type(PexValueType::TemporaryVar), tmpVar(val.var) { }
  PexValue(const Identifier& id) : type(PexValueType::Identifier), s(id.name) {
    if (id.tmpVar != nullptr) {
      type = PexValueType::TemporaryVar;
      tmpVar = id.tmpVar;
    }
  }
  PexValue(const Integer& val) : type(PexValueType::Integer), i(val.i) { }
  PexValue(const Float& val) : type(PexValueType::Float), f(val.f) { }
  PexValue(const Bool& val) : type(PexValueType::Bool), b(val.b) { }
  PexValue(const None& val) : type(PexValueType::None) { }
  PexValue(const Invalid& val) : type(PexValueType::Invalid) { }
  ~PexValue() = default;

  void writeAsm(const PexFile* file, PexAsmWriter& wtr) const;
};

}}
