#include <pex/PexInstruction.h>

#include <limits>
#include <unordered_map>

#include <common/CaselessStringComparer.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace pex {

// Use the opcodes descriptions from the function builder to manage this.
static size_t getArgCountForOpCode(PexOpCode op) {
  switch (op) {
    case PexOpCode::Nop: return 0;
#define OP_ARG1(name, opcode, ...) case PexOpCode::opcode: return 1;
#define OP_ARG2(name, opcode, ...) case PexOpCode::opcode: return 2;
#define OP_ARG3(name, opcode, ...) case PexOpCode::opcode: return 3;
#define OP_ARG4(name, opcode, ...) case PexOpCode::opcode: return 4;
#define OP_ARG5(name, opcode, ...) case PexOpCode::opcode: return 5;
#define OP_ARG6(name, opcode, ...) case PexOpCode::opcode: return 6;
OPCODES(OP_ARG1, OP_ARG2, OP_ARG3, OP_ARG4, OP_ARG5, OP_ARG6)
#undef OP_ARG1
#undef OP_ARG2
#undef OP_ARG3
#undef OP_ARG4
#undef OP_ARG5
#undef OP_ARG6
    case PexOpCode::Invalid:
    case PexOpCode::CallMethod:
    case PexOpCode::CallStatic:
    case PexOpCode::CallParent:
      break;
  }
  CapricaReportingContext::logicalFatal("Unknown PexOpCode!");
}

int32_t PexInstruction::getDestArgIndexForOpCode(PexOpCode op) {
  switch (op) {
    case PexOpCode::Nop:
      return -1;
    case PexOpCode::CallMethod:
    case PexOpCode::CallStatic:
      return 2;
    case PexOpCode::CallParent:
      return 1;
#define OP_ARG1(name, opcode, destArgIdx, ...) case PexOpCode::opcode: return destArgIdx;
#define OP_ARG2(name, opcode, destArgIdx, ...) case PexOpCode::opcode: return destArgIdx;
#define OP_ARG3(name, opcode, destArgIdx, ...) case PexOpCode::opcode: return destArgIdx;
#define OP_ARG4(name, opcode, destArgIdx, ...) case PexOpCode::opcode: return destArgIdx;
#define OP_ARG5(name, opcode, destArgIdx, ...) case PexOpCode::opcode: return destArgIdx;
#define OP_ARG6(name, opcode, destArgIdx, ...) case PexOpCode::opcode: return destArgIdx;
      OPCODES(OP_ARG1, OP_ARG2, OP_ARG3, OP_ARG4, OP_ARG5, OP_ARG6)
#undef OP_ARG1
#undef OP_ARG2
#undef OP_ARG3
#undef OP_ARG4
#undef OP_ARG5
#undef OP_ARG6
    case PexOpCode::Invalid:
      break;
  }
  CapricaReportingContext::logicalFatal("Unknown PexOpCode!");
}

PexInstruction* PexInstruction::read(allocators::ChainedPool* alloc, PexReader& rdr) {
  auto inst = alloc->make<PexInstruction>();
  inst->opCode = (PexOpCode)rdr.read<uint8_t>();

  switch (inst->opCode) {
    case PexOpCode::CallMethod:
    case PexOpCode::CallStatic:
    {
      inst->args.reserve(3);
      for (size_t i = 0; i < 3; i++)
        inst->args.push_back(rdr.read<PexValue>());
      goto CallCommon;
    }
    case PexOpCode::CallParent:
    {
      inst->args.reserve(2);
      for (size_t i = 0; i < 2; i++)
        inst->args.push_back(rdr.read<PexValue>());
      goto CallCommon;
    }
    
    CallCommon:
    {
      auto varVal = rdr.read<PexValue>();
      if (varVal.type != PexValueType::Integer)
        CapricaReportingContext::logicalFatal("The var arg count for call instructions should be an integer!");
      for (size_t i = 0; i < varVal.val.i; i++)
        inst->variadicArgs.push_back(alloc->make<IntrusivePexValue>(rdr.read<PexValue>()));
      break;
    }

    default:
    {
      auto aCount = getArgCountForOpCode(inst->opCode);
      inst->args.reserve(aCount);
      for (size_t i = 0; i < aCount; i++)
        inst->args.push_back(rdr.read<PexValue>());
      break;
    }
  }

  return inst;
}

void PexInstruction::write(PexWriter& wtr) const {
  wtr.write<uint8_t>((uint8_t)opCode);
  for (auto& a : args)
    wtr.write<PexValue>(a);

  switch (opCode) {
    case PexOpCode::CallMethod:
    case PexOpCode::CallParent:
    case PexOpCode::CallStatic:
    {
      assert(variadicArgs.size() <= std::numeric_limits<uint32_t>::max());
      PexValue val{ };
      val.type = PexValueType::Integer;
      val.val.i = (uint32_t)variadicArgs.size();
      wtr.write<PexValue>(val);
      for (auto v : variadicArgs)
        wtr.write<PexValue>(*v);
      break;
    }
    default:
      assert(variadicArgs.size() == 0);
      break;
  }
}

static const caseless_unordered_identifier_map<PexOpCode> opCodeNameMap{
  { "noop", PexOpCode::Nop },
  { "iadd", PexOpCode::IAdd },
  { "fadd", PexOpCode::FAdd },
  { "isubtract", PexOpCode::ISub },
  { "fsubtract", PexOpCode::FSub },
  { "imultiply", PexOpCode::IMul },
  { "fmultiply", PexOpCode::FMul },
  { "idivide", PexOpCode::IDiv },
  { "fdivide", PexOpCode::FDiv },
  { "imod", PexOpCode::IMod },
  { "not", PexOpCode::Not },
  { "inegate", PexOpCode::INeg },
  { "fnegate", PexOpCode::FNeg },
  { "assign", PexOpCode::Assign },
  { "cast", PexOpCode::Cast },
  { "compareeq", PexOpCode::CmpEq },
  { "comparelt", PexOpCode::CmpLt },
  { "comparelte", PexOpCode::CmpLte },
  { "comparegt", PexOpCode::CmpGt },
  { "comparegte", PexOpCode::CmpGte },
  { "jump", PexOpCode::Jmp },
  { "jumpt", PexOpCode::JmpT },
  { "jumpf", PexOpCode::JmpF },
  { "callmethod", PexOpCode::CallMethod },
  { "callparent", PexOpCode::CallParent },
  { "callstatic", PexOpCode::CallStatic },
  { "return", PexOpCode::Return },
  { "strcat", PexOpCode::StrCat },
  { "propget", PexOpCode::PropGet },
  { "propset", PexOpCode::PropSet },
  { "arraycreate", PexOpCode::ArrayCreate },
  { "arraylength", PexOpCode::ArrayLength },
  { "arraygetelement", PexOpCode::ArrayGetElement },
  { "arraysetelement", PexOpCode::ArraySetElement },
  { "arrayfindelement", PexOpCode::ArrayFindElement },
  { "arrayrfindelement", PexOpCode::ArrayRFindElement },
  { "is", PexOpCode::Is },
  { "structcreate", PexOpCode::StructCreate },
  { "structget", PexOpCode::StructGet },
  { "structset", PexOpCode::StructSet },
  { "arrayfindstruct", PexOpCode::ArrayFindStruct },
  { "arrayrfindstruct", PexOpCode::ArrayRFindStruct },
  { "arrayaddelements", PexOpCode::ArrayAdd },
  { "arrayinsertelement", PexOpCode::ArrayInsert },
  { "arrayremovelastelement", PexOpCode::ArrayRemoveLast },
  { "arrayremoveelements", PexOpCode::ArrayRemove },
  { "arrayclearelements", PexOpCode::ArrayClear },
  { "arraygetallmatchingstructs", PexOpCode::ArrayGetAllMatchingStructs },
  { "lockguards", PexOpCode::LockGuards },
  { "unlockguards", PexOpCode::UnlockGuards },
  { "trylockguards", PexOpCode::TryLockGuards },
};

PexOpCode PexInstruction::tryParseOpCode(const std::string& str) {
  auto f = opCodeNameMap.find(str.c_str());
  if (f != opCodeNameMap.end())
    return f->second;
  else
    return PexOpCode::Invalid;
}

static const std::unordered_map<PexOpCode, std::string> opCodeToPexAsmNameMap{
  { PexOpCode::Invalid, "INVALID" },
  { PexOpCode::Nop, "NOOP" },
  { PexOpCode::IAdd, "IADD" },
  { PexOpCode::FAdd, "FADD" },
  { PexOpCode::ISub, "ISUBTRACT" },
  { PexOpCode::FSub, "FSUBTRACT" },
  { PexOpCode::IMul, "IMULTIPLY" },
  { PexOpCode::FMul, "FMULTIPLY" },
  { PexOpCode::IDiv, "IDIVIDE" },
  { PexOpCode::FDiv, "FDIVIDE" },
  { PexOpCode::IMod, "IMOD" },
  { PexOpCode::Not, "NOT" },
  { PexOpCode::INeg, "INEGATE" },
  { PexOpCode::FNeg, "FNEGATE" },
  { PexOpCode::Assign, "ASSIGN" },
  { PexOpCode::Cast, "CAST" },
  { PexOpCode::CmpEq, "COMPAREEQ" },
  { PexOpCode::CmpLt, "COMPARELT" },
  { PexOpCode::CmpLte, "COMPARELTE" },
  { PexOpCode::CmpGt, "COMPAREGT" },
  { PexOpCode::CmpGte, "COMPAREGTE" },
  { PexOpCode::Jmp, "JUMP" },
  { PexOpCode::JmpT, "JUMPT" },
  { PexOpCode::JmpF, "JUMPF" },
  { PexOpCode::CallMethod, "CALLMETHOD" },
  { PexOpCode::CallParent, "CALLPARENT" },
  { PexOpCode::CallStatic, "CALLSTATIC" },
  { PexOpCode::Return, "RETURN" },
  { PexOpCode::StrCat, "STRCAT" },
  { PexOpCode::PropGet, "PROPGET" },
  { PexOpCode::PropSet, "PROPSET" },
  { PexOpCode::ArrayCreate, "ARRAYCREATE" },
  { PexOpCode::ArrayLength, "ARRAYLENGTH" },
  { PexOpCode::ArrayGetElement, "ARRAYGETELEMENT" },
  { PexOpCode::ArraySetElement, "ARRAYSETELEMENT" },
  { PexOpCode::ArrayFindElement, "ARRAYFINDELEMENT" },
  { PexOpCode::ArrayRFindElement, "ARRAYRFINDELEMENT" },
  { PexOpCode::Is, "IS" },
  { PexOpCode::StructCreate, "STRUCTCREATE" },
  { PexOpCode::StructGet, "STRUCTGET" },
  { PexOpCode::StructSet, "STRUCTSET" },
  { PexOpCode::ArrayFindStruct, "ARRAYFINDSTRUCT" },
  { PexOpCode::ArrayRFindStruct, "ARRAYRFINDSTRUCT" },
  { PexOpCode::ArrayAdd, "ARRAYADDELEMENTS" },
  { PexOpCode::ArrayInsert, "ARRAYINSERTELEMENT" },
  { PexOpCode::ArrayRemoveLast, "ARRAYREMOVELASTELEMENT" },
  { PexOpCode::ArrayRemove, "ARRAYREMOVEELEMENTS" },
  { PexOpCode::ArrayClear, "ARRAYCLEARELEMENTS" },
  { PexOpCode::ArrayGetAllMatchingStructs, "ARRAYGETALLMATCHINGSTRUCTS" },
  { PexOpCode::LockGuards, "LOCKGUARDS" },
  { PexOpCode::UnlockGuards, "UNLOCKGUARDS" },
  { PexOpCode::TryLockGuards, "TRYLOCKGUARDS" },
};

std::string PexInstruction::opCodeToPexAsm(PexOpCode op) {
  auto f = opCodeToPexAsmNameMap.find(op);
  if (f != opCodeToPexAsmNameMap.end())
    return f->second;
  else
    CapricaReportingContext::logicalFatal("Unknown PexOpCode '%u'!", (unsigned)op);
}

}}
