#pragma once

#include <cstdint>
#include <string>

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
  ArrayGetAllMatchingStructs,
  LockGuards,
  UnlockGuards,
  TryLockGuards,
  MAXOPCODE
};

#define MAX_OPCODE_RAW_ARGS 6

using PexInstructionArgs = boost::container::static_vector<caprica::pex::PexValue, MAX_OPCODE_RAW_ARGS>;

struct PexInstruction final
{
  static constexpr size_t kMaxRawArgs = MAX_OPCODE_RAW_ARGS;

  PexOpCode opCode{ PexOpCode::Nop };
  PexInstructionArgs args{ };
  IntrusiveLinkedList<IntrusivePexValue> variadicArgs{ };

  explicit PexInstruction() = default;
  explicit PexInstruction(PexOpCode op) : opCode(op) { assert(op == PexOpCode::Nop); }
  explicit PexInstruction(PexOpCode op, PexValue arg1) : opCode(op), args({ arg1 }) { }
  explicit PexInstruction(PexOpCode op, PexValue arg1, PexValue arg2) : opCode(op), args({ arg1, arg2 }) { }
  explicit PexInstruction(PexOpCode op, PexValue arg1, PexValue arg2, IntrusiveLinkedList<IntrusivePexValue>&& varArguments) : opCode(op), args({ arg1, arg2 }), variadicArgs(std::move(varArguments)) { }
  explicit PexInstruction(PexOpCode op, PexValue arg1, PexValue arg2, PexValue arg3) : opCode(op), args({ arg1, arg2, arg3 }) { }
  explicit PexInstruction(PexOpCode op, PexValue arg1, PexValue arg2, PexValue arg3, IntrusiveLinkedList<IntrusivePexValue>&& varArguments) : opCode(op), args({ arg1, arg2, arg3 }), variadicArgs(std::move(varArguments)) { }
  explicit PexInstruction(PexOpCode op, PexValue arg1, PexValue arg2, PexValue arg3, PexValue arg4) : opCode(op), args({ arg1, arg2, arg3, arg4 }) { }
  explicit PexInstruction(PexOpCode op, PexValue arg1, PexValue arg2, PexValue arg3, PexValue arg4, PexValue arg5) : opCode(op), args({ arg1, arg2, arg3, arg4, arg5 }) { }
  explicit PexInstruction(PexOpCode op, PexValue arg1, PexValue arg2, PexValue arg3, PexValue arg4, PexValue arg5, PexValue arg6) : opCode(op), args({ arg1, arg2, arg3, arg4, arg5, arg6 }) { }
  explicit PexInstruction(PexOpCode op, PexInstructionArgs&& arguments) : opCode(op), args(std::move(arguments)) { }
  explicit PexInstruction(PexOpCode op, PexInstructionArgs&& arguments, IntrusiveLinkedList<IntrusivePexValue>&& varArguments) : opCode(op), args(std::move(arguments)), variadicArgs(std::move(varArguments)) { }
  explicit PexInstruction(PexOpCode op, PexValue arg1, IntrusiveLinkedList<IntrusivePexValue>&& varArguments) : opCode(op), args({ arg1 }), variadicArgs(std::move(varArguments)) { }
  explicit PexInstruction(PexOpCode op, IntrusiveLinkedList<IntrusivePexValue>&& varArguments) : opCode(op), args(), variadicArgs(std::move(varArguments)) { }

    PexInstruction(const PexInstruction&) = default;
  PexInstruction(PexInstruction&&) = default;
  PexInstruction& operator=(const PexInstruction&) = default;
  PexInstruction& operator=(PexInstruction&&) = default;
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
      return args[0].val.i;
    } else if (opCode == PexOpCode::JmpT || opCode == PexOpCode::JmpF) {
      assert(args.size() == 2);
      assert(args[1].type == PexValueType::Integer);
      return args[1].val.i;
    }
    CapricaReportingContext::logicalFatal("Attempted to get the branch target of a non-branch opcode!");
  }

  void setBranchTarget(int target) {
    if (opCode == PexOpCode::Jmp) {
      assert(args.size() == 1);
      assert(args[0].type == PexValueType::Integer);
      args[0].val.i = target;
    } else if (opCode == PexOpCode::JmpT || opCode == PexOpCode::JmpF) {
      assert(args.size() == 2);
      assert(args[1].type == PexValueType::Integer);
      args[1].val.i = target;
    } else {
      CapricaReportingContext::logicalFatal("Attempted to get the branch target of a non-branch opcode!");
    }
  }
  static int32_t getDestArgIndexForOpCode(PexOpCode op);

  static PexInstruction* read(allocators::ChainedPool* alloc, PexReader& rdr);
  void write(PexWriter& wtr) const;

  static PexOpCode tryParseOpCode(const std::string& str);
  static std::string opCodeToPexAsm(PexOpCode op);

private:
  friend IntrusiveLinkedList<PexInstruction>;
  PexInstruction* next{ nullptr };
};

}}
