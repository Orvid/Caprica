#include <pex/PexInstruction.h>

#include <limits>
#include <map>

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
  { "arrayadd", PexOpCode::ArrayAdd },
  { "arrayinsert", PexOpCode::ArrayInsert },
  { "arrayremovelast", PexOpCode::ArrayRemoveLast },
  { "arrayremove", PexOpCode::ArrayRemove },
  { "arrayclear", PexOpCode::ArrayClear },
};

PexOpCode PexInstruction::tryParseOpCode(const std::string& str) {
  auto f = opCodeNameMap.find(str);
  if (f != opCodeNameMap.end())
    return f->second;
  else
    return PexOpCode::Invalid;
}

}}