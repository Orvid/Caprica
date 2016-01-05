#include <pex/PexFunction.h>

#include <unordered_map>

#include <pex/PexFile.h>
#include <pex/PexObject.h>
#include <pex/PexState.h>

namespace caprica { namespace pex {

PexFunction* PexFunction::read(PexReader& rdr, bool isProperty) {
  auto func = new PexFunction();
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
  func->parameters.reserve(pSize);
  for (size_t i = 0; i < pSize; i++)
    func->parameters.push_back(PexFunctionParameter::read(rdr));
  auto lSize = rdr.read<uint16_t>();
  func->locals.reserve(lSize);
  for (size_t i = 0; i < lSize; i++)
    func->locals.push_back(PexLocalVariable::read(rdr));
  auto iSize = rdr.read<uint16_t>();
  func->instructions.reserve(iSize);
  for (size_t i = 0; i < iSize; i++)
    func->instructions.push_back(PexInstruction::read(rdr));

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
    wtr.write(file->getStringValue(name).c_str());
  if (isNative)
    wtr.write(" native");
  if (isGlobal)
    wtr.write(" static");
  wtr.writeln();
  wtr.ident++;

  wtr.writeKV<PexUserFlags>("userFlags", userFlags);
  wtr.writeKV<std::string>("docString", file->getStringValue(documentationString));
  wtr.writeln(".return %s", file->getStringValue(returnTypeName).c_str());

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

    PexDebugFunctionInfo* debInf{ nullptr };
    if (file->debugInfo) {
      auto objName = file->getStringValue(obj->name);
      auto stateName = state ? file->getStringValue(state->name) : "";
      auto fName = propName == "" ? file->getStringValue(name) : propName;

      for (auto fi : file->debugInfo->functions) {
        if (file->getStringValue(fi->objectName) == objName && file->getStringValue(fi->stateName) == stateName && file->getStringValue(fi->functionName) == fName && fi->functionType == funcType) {
          debInf = fi;
          break;
        }
      }
    }

    // This is the fun part.
    std::unordered_map<size_t, size_t> labelMap;
    for (size_t i = 0; i < instructions.size(); i++) {
      if (instructions[i]->opCode == PexOpCode::Jmp) {
        auto targI = instructions[i]->args[0].i + i;
        if (!labelMap.count((size_t)(targI)))
          labelMap.insert({ (size_t)targI, labelMap.size() });
      } else if (instructions[i]->opCode == PexOpCode::JmpT || instructions[i]->opCode == PexOpCode::JmpF) {
        auto targI = instructions[i]->args[1].i + i;
        if (!labelMap.count((size_t)(targI)))
          labelMap.insert({ (size_t)targI, labelMap.size() });
      }
    }

    for (size_t i = 0; i < instructions.size(); i++) {
      auto f = labelMap.find(i);
      if (f != labelMap.end()) {
        wtr.writeln("label%llu:", f->second);
      }

      wtr.write(PexInstruction::opCodeToPexAsm(instructions[i]->opCode));

      if (instructions[i]->opCode == PexOpCode::Jmp) {
        wtr.write(" label%llu", labelMap[(size_t)(instructions[i]->args[0].i + i)]);
      } else if (instructions[i]->opCode == PexOpCode::JmpT || instructions[i]->opCode == PexOpCode::JmpF) {
        wtr.write(" ");
        instructions[i]->args[0].writeAsm(file, wtr);
        wtr.write(" label%llu", labelMap[(size_t)(instructions[i]->args[1].i + i)]);
      } else {
        for (auto& a : instructions[i]->args) {
          wtr.write(" ");
          a.writeAsm(file, wtr);
        }
        for (auto& a : instructions[i]->variadicArgs) {
          wtr.write(" ");
          a.writeAsm(file, wtr);
        }
      }

      if (debInf) {
        wtr.write(" ;@line %u", debInf->instructionLineMap[i]);
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
