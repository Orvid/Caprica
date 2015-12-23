#pragma once

#include <cstdint>
#include <vector>

#include <pex/PexValue.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

enum class PexOpCode : uint8_t
{
  Nop = 0,
  IAdd,
  FAdd,
  ISub,
  FSub,
  IMul,
  FMul,
  IDiv,
  FDiv,
  IMod,
  Not,
  INeg,
  FNeg,
  Assign,
  Cast,
  CmpEq,
  CmpLt,
  CmpLte,
  CmpGt,
  CmpGte,
  Jmp,
  JmpT,
  JmpF,
  CallMethod,
  CallParent,
  CallStatic,
  Return,
  StrCat,
  PropGet,
  PropSet,
  ArrayCreate,
  ArrayLength,
  ArrayGetElement,
  ArraySetElement,
  ArrayFindElement,
  ArrayRFindElement,
  Is,
  StructCreate,
  StructGet,
  StructSet,
  ArrayFindStruct,
  ArrayRFindStruct,
  ArrayAdd,
  ArrayInsert,
  ArrayRemoveLast,
  ArrayRemove,
  ArrayClear,
};

struct PexInstruction final
{
  PexOpCode opCode{ PexOpCode::Nop };
  std::vector<PexValue> args{ };
  std::vector<PexValue> variadicArgs{ };

  PexInstruction() = default;
  PexInstruction(PexOpCode op) : opCode(op) { assert(op == PexOpCode::Nop); }
  PexInstruction(PexOpCode op, std::vector<PexValue> arguments) : opCode(op), args(arguments) { }
  PexInstruction(PexOpCode op, std::vector<PexValue> arguments, std::vector<PexValue> varArguments) : opCode(op), args(arguments), variadicArgs(varArguments) { }
  ~PexInstruction() = default;

  void write(PexWriter& wtr) const;
};

}}
