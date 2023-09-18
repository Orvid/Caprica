#include <pex/PexFunction.h>

#include <unordered_map>

#include <pex/PexFile.h>
#include <pex/PexObject.h>
#include <pex/PexState.h>

namespace caprica { namespace pex {

PexFunction * PexFunction::read(allocators::ChainedPool *alloc, PexReader &rdr, bool isProperty, GameID gameType) {
  auto func = alloc->make<PexFunction>();
  if (!isProperty)
    func->name = rdr.read<PexString>();
  func->returnTypeName = rdr.read<PexString>();
  func->documentationString = rdr.read<PexString>();
  func->userFlags = rdr.read<PexUserFlags>();
  auto flags = rdr.read<uint8_t>();
  if (flags & 0x01)
    func->isGlobal = true;
  if (flags & 0x02)
    func->isNative = true;

  auto pSize = rdr.read<uint16_t>();
  for (size_t i = 0; i < pSize; i++)
    func->parameters.push_back(PexFunctionParameter::read(alloc, rdr));
  auto lSize = rdr.read<uint16_t>();
  for (size_t i = 0; i < lSize; i++)
    func->locals.push_back(PexLocalVariable::read(alloc, rdr));
  auto iSize = rdr.read<uint16_t>();
  for (size_t i = 0; i < iSize; i++)
    func->instructions.push_back(PexInstruction::read(alloc, rdr, gameType));

  return func;
}

void PexFunction::write(PexWriter& wtr) const {
  if (name.valid())
    wtr.write<PexString>(name);
  wtr.write<PexString>(returnTypeName);
  wtr.write<PexString>(documentationString);
  wtr.write<PexUserFlags>(userFlags);
  uint8_t flags = 0;
  if (isGlobal)
    flags |= 1;
  if (isNative)
    flags |= 2;
  wtr.write<uint8_t>(flags);

  wtr.boundWrite<uint16_t>(parameters.size());
  for (auto p : parameters)
    p->write(wtr);
  wtr.boundWrite<uint16_t>(locals.size());
  for (auto l : locals)
    l->write(wtr);
  wtr.boundWrite<uint16_t>(instructions.size());
  for (auto i : instructions)
    i->write(wtr);
}

void PexFunction::writeAsm(const PexFile* file, const PexObject* obj, const PexState* state, PexDebugFunctionType funcType, std::string propName, PexAsmWriter& wtr) const {
  wtr.write(".function ");
  if (funcType == PexDebugFunctionType::Getter)
    wtr.write("get");
  else if (funcType == PexDebugFunctionType::Setter)
    wtr.write("set");
  else
    wtr.write(file->getStringValue(name).to_string().c_str());

  if (isNative)
    wtr.write(" native");
  if (isGlobal)
    wtr.write(" static");

  wtr.writeln();
  wtr.ident++;

  wtr.writeKV<PexUserFlags>("userFlags", userFlags);
  wtr.writeKV<std::string>("docString", file->getStringValue(documentationString).to_string());
  wtr.writeln(".return %s", file->getStringValue(returnTypeName).to_string().c_str());

  wtr.writeln(".paramTable");
  wtr.ident++;
  for (auto p : parameters)
    p->writeAsm(file, wtr);
  wtr.ident--;
  wtr.writeln(".endParamTable");

  if (!isNative) {
    wtr.writeln(".localTable");
    wtr.ident++;
    for (auto l : locals)
      l->writeAsm(file, wtr);
    wtr.ident--;
    wtr.writeln(".endLocalTable");

    wtr.writeln(".code");
    wtr.ident++;

    auto debInf = file->tryFindFunctionDebugInfo(obj, state, this, propName, funcType);

    // This is the fun part.
    std::unordered_map<size_t, size_t> labelMap;
    for (auto cur = instructions.begin(), end = instructions.end(); cur != end; ++cur) {
      if (cur->isBranch()) {
        auto targI = cur->branchTarget() + cur.index;
        if (!labelMap.count((size_t)(targI)))
          labelMap.emplace((size_t)targI, labelMap.size());
      }
    }

    for (auto cur = instructions.begin(), end = instructions.end(); cur != end; ++cur) {
      auto f = labelMap.find(cur.index);
      if (f != labelMap.end()) {
        wtr.writeln("label%llu:", f->second);
      }

      wtr.write(PexInstruction::opCodeToPexAsm(cur->opCode));

      if (cur->opCode == PexOpCode::Jmp) {
        wtr.write(" label%llu", labelMap[(size_t)(cur->args[0].val.i + cur.index)]);
      } else if (cur->opCode == PexOpCode::JmpT || cur->opCode == PexOpCode::JmpF) {
        wtr.write(" ");
        cur->args[0].writeAsm(file, wtr);
        wtr.write(" label%llu", labelMap[(size_t)(cur->args[1].val.i + cur.index)]);
      } else {
        for (auto& a : cur->args) {
          wtr.write(" ");
          a.writeAsm(file, wtr);
        }
        for (auto& a : cur->variadicArgs) {
          wtr.write(" ");
          a->writeAsm(file, wtr);
        }
      }

      if (debInf && cur.index < debInf->instructionLineMap.size()) {
        wtr.write(" ;@line %u", debInf->instructionLineMap[cur.index]);
      }

      wtr.writeln();
    }

    auto f = labelMap.find(instructions.size());
    if (f != labelMap.end()) {
      wtr.writeln("label%llu:", f->second);
    }

    wtr.ident--;
    wtr.writeln(".endCode");
  }

  wtr.ident--;
  wtr.writeln(".endFunction");
}

}}
