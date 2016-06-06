#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <boost/container/static_vector.hpp>

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

using PexInstructionArgs = boost::container::static_vector<caprica::pex::PexValue, 5>;

struct PexInstruction final
{
  static constexpr size_t kMaxRawArgs = 5;

  PexOpCode opCode{ PexOpCode::Nop };
  PexInstructionArgs args{ };
  std::vector<PexValue> variadicArgs{ };

  explicit PexInstruction() = default;
  explicit PexInstruction(PexOpCode op) : opCode(op) { assert(op == PexOpCode::Nop); }
  explicit PexInstruction(PexOpCode op, PexInstructionArgs arguments) : opCode(op), args(arguments) { }
  explicit PexInstruction(PexOpCode op, PexInstructionArgs arguments, std::vector<PexValue>&& varArguments) : opCode(op), args(arguments), variadicArgs(std::move(varArguments)) { }
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
    CapricaReportingContext::logicalFatal("Attempted to get the branch target of a non-branch opcode!");
  }

  void setBranchTarget(int target) {
    if (opCode == PexOpCode::Jmp) {
      assert(args.size() == 1);
      assert(args[0].type == PexValueType::Integer);
      args[0].i = target;
    } else if (opCode == PexOpCode::JmpT || opCode == PexOpCode::JmpF) {
      assert(args.size() == 2);
      assert(args[1].type == PexValueType::Integer);
      args[1].i = target;
    } else {
      CapricaReportingContext::logicalFatal("Attempted to get the branch target of a non-branch opcode!");
    }
  }
  int32_t getDestArgIndex() const;

  static PexInstruction* read(PexReader& rdr);
  void write(PexWriter& wtr) const;

  static PexOpCode tryParseOpCode(const std::string& str);
  static std::string opCodeToPexAsm(PexOpCode op);
};

}}
