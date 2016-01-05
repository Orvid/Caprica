#include <pex/PexInstruction.h>

#include <limits>
#include <map>
#include <unordered_map>

#include <papyrus/parser/PapyrusLexer.h>

namespace caprica { namespace pex {

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
      val.i = (uint32_t)variadicArgs.size();
      wtr.write<PexValue>(val);
      for (auto& v : variadicArgs)
        wtr.write<PexValue>(v);
      break;
    }
    default:
      assert(variadicArgs.size() == 0);
      break;
  }
}

static const std::map<std::string, PexOpCode, papyrus::parser::CaselessStringComparer> opCodeNameMap{
  { "noop", PexOpCode::Nop },
  { "iadd", PexOpCode::IAdd },
  { "fadd", PexOpCode::FAdd },
  { "isub", PexOpCode::ISub },
  { "fsub", PexOpCode::FSub },
  { "imul", PexOpCode::IMul },
  { "fmul", PexOpCode::FMul },
  { "idiv", PexOpCode::IDiv },
  { "fdiv", PexOpCode::FDiv },
  { "imod", PexOpCode::IMod },
  { "not", PexOpCode::Not },
  { "ineg", PexOpCode::INeg },
  { "fneg", PexOpCode::FNeg },
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
};

PexOpCode PexInstruction::tryParseOpCode(const std::string& str) {
  auto f = opCodeNameMap.find(str);
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
  { PexOpCode::ISub, "ISUB" },
  { PexOpCode::FSub, "FSUB" },
  { PexOpCode::IMul, "IMUL" },
  { PexOpCode::FMul, "FMUL" },
  { PexOpCode::IDiv, "IDIV" },
  { PexOpCode::FDiv, "FDIV" },
  { PexOpCode::IMod, "IMOD" },
  { PexOpCode::Not, "NOT" },
  { PexOpCode::INeg, "INEG" },
  { PexOpCode::FNeg, "FNEG" },
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
};

std::string PexInstruction::opCodeToPexAsm(PexOpCode op) {
  auto f = opCodeToPexAsmNameMap.find(op);
  if (f != opCodeToPexAsmNameMap.end())
    return f->second;
  else
    CapricaError::logicalFatal("Unknown PexOpCode '%u'!", (unsigned)op);
}

}}
