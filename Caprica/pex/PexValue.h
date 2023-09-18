#pragma once

#include <cassert>
#include <cstdint>

#include <common/IntrusiveLinkedList.h>

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

private:
  friend IntrusiveLinkedList<PexTemporaryVariableRef>;
  PexTemporaryVariableRef* next{ nullptr };
};

struct PexValue
{
  PexValueType type{ PexValueType::None };
  union ValueData
  {
    PexString s;
    int32_t i;
    float f;
    bool b;
    PexLabel* l;
    PexTemporaryVariableRef* tmpVar;

    ValueData() : tmpVar(nullptr) { }
    ValueData(PexString str) : s(str) { }
    ValueData(int32_t it) : i(it) { }
    ValueData(float fl) : f(fl) { }
    ValueData(bool bl) : b(bl) { }
    ValueData(PexLabel* label) : l(label) { }
    ValueData(PexTemporaryVariableRef* ref) : tmpVar(ref) { }
  } val;

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
        return Identifier(var.val.tmpVar);
      assert(var.type == PexValueType::Identifier);
      return Identifier(var.val.s);
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
  PexValue(PexLabel* lab) : type(PexValueType::Label), val(lab) { }
  PexValue(PexLocalVariable* var) : type(PexValueType::Identifier), val(var->name) { }
  PexValue(const TemporaryVariable& val) : type(PexValueType::TemporaryVar), val(val.var) { }
  PexValue(const Identifier& id) : type(PexValueType::Identifier), val(id.name) {
    if (id.tmpVar != nullptr) {
      type = PexValueType::TemporaryVar;
      val.tmpVar = id.tmpVar;
    }
  }
  PexValue(const Integer& val) : type(PexValueType::Integer), val(val.i) { }
  PexValue(const Float& val) : type(PexValueType::Float), val(val.f) { }
  PexValue(const Bool& val) : type(PexValueType::Bool), val(val.b) { }
  PexValue(const None&) : type(PexValueType::None) { }
  PexValue(const Invalid&) : type(PexValueType::Invalid) { }
  PexValue(const PexValue&) = default;
  PexValue(PexValue&& other) = default;
  PexValue& operator =(const PexValue&) = default;
  PexValue& operator =(PexValue&&) = default;
  ~PexValue() = default;

  void writeAsm(const PexFile* file, PexAsmWriter& wtr) const;

  bool operator ==(const PexValue& other) const;
  bool operator !=(const PexValue& other) const {
    return !(*this == other);
  }
};

struct IntrusivePexValue final : public PexValue
{
  IntrusivePexValue(const PexValue& str) : PexValue(str) { }
  IntrusivePexValue(PexValue&& str) : PexValue(str) { }

private:
  friend IntrusiveLinkedList<IntrusivePexValue>;
  IntrusivePexValue* next{ nullptr };
};

}}
