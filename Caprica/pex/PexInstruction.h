#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <pex/PexReader.h>
#include <pex/PexValue.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

enum class PexOpCode : uint8_t
{
  Invalid = 0xFF,

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

  static PexInstruction* read(PexReader& rdr);
  void write(PexWriter& wtr) const;

  static PexOpCode tryParseOpCode(const std::string& str);
  static std::string opCodeToPexAsm(PexOpCode op);
};

}}
