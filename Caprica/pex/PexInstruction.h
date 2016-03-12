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

  explicit PexInstruction() = default;
  explicit PexInstruction(PexOpCode op) : opCode(op) { assert(op == PexOpCode::Nop); }
  explicit PexInstruction(PexOpCode op, std::vector<PexValue> arguments) : opCode(op), args(arguments) { }
  explicit PexInstruction(PexOpCode op, std::vector<PexValue> arguments, std::vector<PexValue> varArguments) : opCode(op), args(arguments), variadicArgs(varArguments) { }
  PexInstruction(const PexInstruction&) = delete;
  ~PexInstruction() = default;

  bool isBranch() const noexcept {
    return opCode == PexOpCode::Jmp || opCode == PexOpCode::JmpT || opCode == PexOpCode::JmpF;
  }

  /**
   * The target of this branch instruction, as a
   * number representing the relative target in
   * number of instructions.
   */
  int branchTarget() const {
    if (opCode == PexOpCode::Jmp) {
      assert(args.size() == 1);
      assert(args[0].type == PexValueType::Integer);
      return args[0].i;
    } else if (opCode == PexOpCode::JmpT || opCode == PexOpCode::JmpF) {
      assert(args.size() == 2);
      assert(args[1].type == PexValueType::Integer);
      return args[1].i;
    }
    CapricaError::logicalFatal("Attempted to get the branch target of a non-branch opcode!");
  }

  void makeNop() {
    opCode = PexOpCode::Nop;
    args.clear();
    variadicArgs.clear();
  }

  static PexInstruction* read(PexReader& rdr);
  void write(PexWriter& wtr) const;

  static PexOpCode tryParseOpCode(const std::string& str);
  static std::string opCodeToPexAsm(PexOpCode op);
};

}}
