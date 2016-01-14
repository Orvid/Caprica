#pragma once

#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <vector>

#include <common/CapricaFileLocation.h>
#include <papyrus/PapyrusType.h>

#include <pex/PexDebugFunctionInfo.h>
#include <pex/PexFunction.h>
#include <pex/PexInstruction.h>
#include <pex/PexLabel.h>
#include <pex/PexLocalVariable.h>
#include <pex/PexString.h>

namespace caprica { namespace pex {

// We use this rather than repeating this information in multiple places.
#define OPCODES(ARG1, ARG2, ARG3, ARG4, ARG5) \
  ARG3(iadd, IAdd, 0, PexValue::Identifier, dest, PexValue, arg1, PexValue, arg2) \
  ARG3(fadd, FAdd, 0, PexValue::Identifier, dest, PexValue, arg1, PexValue, arg2) \
  ARG3(isub, ISub, 0, PexValue::Identifier, dest, PexValue, arg1, PexValue, arg2) \
  ARG3(fsub, FSub, 0, PexValue::Identifier, dest, PexValue, arg1, PexValue, arg2) \
  ARG3(imul, IMul, 0, PexValue::Identifier, dest, PexValue, arg1, PexValue, arg2) \
  ARG3(fmul, FMul, 0, PexValue::Identifier, dest, PexValue, arg1, PexValue, arg2) \
  ARG3(idiv, IDiv, 0, PexValue::Identifier, dest, PexValue, arg1, PexValue, arg2) \
  ARG3(fdiv, FDiv, 0, PexValue::Identifier, dest, PexValue, arg1, PexValue, arg2) \
  ARG3(imod, IMod, 0, PexValue::Identifier, dest, PexValue, arg1, PexValue, arg2) \
  ARG2(not, Not, 0, PexValue::Identifier, dest, PexValue, src) \
  ARG2(ineg, INeg, 0, PexValue::Identifier, dest, PexValue, src) \
  ARG2(fneg, FNeg, 0, PexValue::Identifier, dest, PexValue, src) \
  ARG2(assign, Assign, 0, PexValue::Identifier, dest, PexValue, src) \
  ARG2(cast, Cast, 0, PexValue::Identifier, dest, PexValue, src) \
  ARG3(cmpeq, CmpEq, 0, PexValue::Identifier, dest, PexValue, arg1, PexValue, arg2) \
  ARG3(cmplt, CmpLt, 0, PexValue::Identifier, dest, PexValue, arg1, PexValue, arg2) \
  ARG3(cmplte, CmpLte, 0, PexValue::Identifier, dest, PexValue, arg1, PexValue, arg2) \
  ARG3(cmpgt, CmpGt, 0, PexValue::Identifier, dest, PexValue, arg1, PexValue, arg2) \
  ARG3(cmpgte, CmpGte, 0, PexValue::Identifier, dest, PexValue, arg1, PexValue, arg2) \
  ARG1(jmp, Jmp, -1, PexLabel*, target) \
  ARG2(jmpt, JmpT, -1, PexValue, cond, PexLabel*, target) \
  ARG2(jmpf, JmpF, -1, PexValue, cond, PexLabel*, target) \
  ARG1(ret, Return, -1, PexValue, val) \
  ARG3(strcat, StrCat, 0, PexValue::Identifier, dest, PexValue, arg1, PexValue, arg2) \
  ARG3(propget, PropGet, 2, PexString, propName, PexValue::Identifier, baseObj, PexValue::Identifier, dest) \
  ARG3(propset, PropSet, -1, PexString, propName, PexValue::Identifier, baseObj, PexValue, val) \
  ARG2(arraycreate, ArrayCreate, 0, PexValue::Identifier, dest, PexValue, length) \
  ARG2(arraylength, ArrayLength, 0, PexValue::Identifier, dest, PexValue::Identifier, arr) \
  ARG3(arraygetelement, ArrayGetElement, 0, PexValue::Identifier, dest, PexValue::Identifier, arr, PexValue, index) \
  ARG3(arraysetelement, ArraySetElement, -1, PexValue::Identifier, arr, PexValue, index, PexValue, val) \
  ARG4(arrayfindelement, ArrayFindElement, 1, PexValue::Identifier, arr, PexValue::Identifier, dest, PexValue, element, PexValue, startIndex) \
  ARG4(arrayrfindelement, ArrayRFindElement, 1, PexValue::Identifier, arr, PexValue::Identifier, dest, PexValue, element, PexValue, startIndex) \
  ARG3(is, Is, 0, PexValue::Identifier, dest, PexValue, src, PexValue::Identifier, type) \
  ARG1(structcreate, StructCreate, 0, PexValue::Identifier, dest) \
  ARG3(structget, StructGet, 0, PexValue::Identifier, dest, PexValue::Identifier, baseObj, PexString, memberName) \
  ARG3(structset, StructSet, -1, PexValue::Identifier, baseObj, PexString, memberName, PexValue, val) \
  ARG5(arrayfindstruct, ArrayFindStruct, 1, PexValue::Identifier, baseObj, PexValue::Identifier, dest, PexValue, memberName, PexValue, valueToSearchFor, PexValue, startIndex) \
  ARG5(arrayrfindstruct, ArrayRFindStruct, 1, PexValue::Identifier, baseObj, PexValue::Identifier, dest, PexValue, memberName, PexValue, valueToSearchFor, PexValue, startIndex) \
  ARG3(arrayadd, ArrayAdd, -1, PexValue::Identifier, baseObj, PexValue, element, PexValue, count) \
  ARG3(arrayinsert, ArrayInsert, -1, PexValue::Identifier, baseObj, PexValue, element, PexValue, index) \
  ARG1(arrayremovelast, ArrayRemoveLast, -1, PexValue::Identifier, arr) \
  ARG3(arrayremove, ArrayRemove, -1, PexValue::Identifier, baseObj, PexValue, index, PexValue, count) \
  ARG1(arrayclear, ArrayClear, -1, PexValue::Identifier, arr) \

namespace op {

struct nop final { };

#define OP_ARG1(name, opcode, destArgIdx, argType1, argName1) \
struct name final { \
  PexValue a1; \
  name(argType1 argName1) : a1(argName1) { } \
};
#define OP_ARG2(name, opcode, destArgIdx, argType1, argName1, argType2, argName2) \
struct name final { \
  PexValue a1, a2; \
  name(argType1 argName1, argType2 argName2) : a1(argName1), a2(argName2) { } \
};
#define OP_ARG3(name, opcode, destArgIdx, argType1, argName1, argType2, argName2, argType3, argName3) \
struct name final { \
  PexValue a1, a2, a3; \
  name(argType1 argName1, argType2 argName2, argType3 argName3) : a1(argName1), a2(argName2), a3(argName3) { } \
};
#define OP_ARG4(name, opcode, destArgIdx, argType1, argName1, argType2, argName2, argType3, argName3, argType4, argName4) \
struct name final { \
  PexValue a1, a2, a3, a4; \
  name(argType1 argName1, argType2 argName2, argType3 argName3, argType4 argName4) : a1(argName1), a2(argName2), a3(argName3), a4(argName4) { } \
};
#define OP_ARG5(name, opcode, destArgIdx, argType1, argName1, argType2, argName2, argType3, argName3, argType4, argName4, argType5, argName5) \
struct name final { \
  PexValue a1, a2, a3, a4, a5; \
  name(argType1 argName1, argType2 argName2, argType3 argName3, argType4 argName4, argType5 argName5) : a1(argName1), a2(argName2), a3(argName3), a4(argName4), a5(argName5) { } \
};
OPCODES(OP_ARG1, OP_ARG2, OP_ARG3, OP_ARG4, OP_ARG5)
#undef OP_ARG1
#undef OP_ARG2
#undef OP_ARG3
#undef OP_ARG4
#undef OP_ARG5

struct callmethod final
{
  PexValue a1, a2, a3;
  std::vector<PexValue> variadicArgs;
  callmethod(PexString function, PexValue baseObj, PexValue::Identifier dest, std::vector<PexValue> varArgs) : a1(function), a2(baseObj), a3(dest), variadicArgs(varArgs) { }
};

struct callparent final
{
  PexValue a1, a2;
  std::vector<PexValue> variadicArgs;
  callparent(PexString function, PexValue::Identifier dest, std::vector<PexValue> varArgs) : a1(function), a2(dest), variadicArgs(varArgs) { }
};

struct callstatic final
{
  PexValue a1, a2, a3;
  std::vector<PexValue> variadicArgs;
  callstatic(PexString type, PexString function, PexValue::Identifier dest, std::vector<PexValue> varArgs) : a1(type), a2(function), a3(dest), variadicArgs(varArgs) { }
};
  
}

struct PexFunctionBuilder final
{
  PexFunctionBuilder& operator <<(op::nop& instr) { return push(PexOpCode::Nop); }

#define OP_ARG1(name, opcode, destArgIdx, at1, an1) \
PexFunctionBuilder& operator <<(op::name& instr) { return push(PexOpCode::opcode, instr.a1); }
#define OP_ARG2(name, opcode, destArgIdx, at1, an1, at2, an2) \
PexFunctionBuilder& operator <<(op::name& instr) { return push(PexOpCode::opcode, instr.a1, instr.a2); }
#define OP_ARG3(name, opcode, destArgIdx, at1, an1, at2, an2, at3, an3) \
PexFunctionBuilder& operator <<(op::name& instr) { return push(PexOpCode::opcode, instr.a1, instr.a2, instr.a3); }
#define OP_ARG4(name, opcode, destArgIdx, at1, an1, at2, an2, at3, an3, at4, an4) \
PexFunctionBuilder& operator <<(op::name& instr) { return push(PexOpCode::opcode, instr.a1, instr.a2, instr.a3, instr.a4); }
#define OP_ARG5(name, opcode, destArgIdx, at1, an1, at2, an2, at3, an3, at4, an4, at5, an5) \
PexFunctionBuilder& operator <<(op::name& instr) { return push(PexOpCode::opcode, instr.a1, instr.a2, instr.a3, instr.a4, instr.a5); }
  OPCODES(OP_ARG1, OP_ARG2, OP_ARG3, OP_ARG4, OP_ARG5)
#undef OP_ARG1
#undef OP_ARG2
#undef OP_ARG3
#undef OP_ARG4
#undef OP_ARG5

  PexFunctionBuilder& operator <<(op::callmethod& instr) {
    return push(new PexInstruction(PexOpCode::CallMethod, std::vector<PexValue>{ instr.a1, instr.a2, instr.a3 }, instr.variadicArgs));
  }
  PexFunctionBuilder& operator <<(op::callparent& instr) {
    return push(new PexInstruction(PexOpCode::CallParent, std::vector<PexValue>{ instr.a1, instr.a2 }, instr.variadicArgs));
  }
  PexFunctionBuilder& operator <<(op::callstatic& instr) {
    return push(new PexInstruction(PexOpCode::CallStatic, std::vector<PexValue>{ instr.a1, instr.a2, instr.a3 }, instr.variadicArgs));
  }

  PexFunctionBuilder& operator <<(const CapricaFileLocation& loc) {
    currentLocation = loc;
    return *this;
  }

  PexLocalVariable* allocateLocal(const std::string& name, const papyrus::PapyrusType& tp) {
    auto loc = new PexLocalVariable();
    loc->name = file->getString(name);
    loc->type = tp.buildPex(file);
    locals.push_back(loc);
    return loc;
  }

  PexLocalVariable* getNoneLocal(const CapricaFileLocation& location) {
    for (auto& loc : locals) {
      if (file->getStringValue(loc->name) == "::nonevar")
        return loc;
    }
    return allocateLocal("::nonevar", papyrus::PapyrusType::None(location));
  }

  PexLocalVariable* allocLongLivedTemp(const papyrus::PapyrusType& tp) {
    auto v = internalAllocateTempVar(tp.buildPex(file));
    longLivedTempVars.insert(v->name);
    return v;
  }

  void freeLongLivedTemp(PexLocalVariable* loc) {
    longLivedTempVars.erase(loc->name);
    return freeValueIfTemp(PexValue::Identifier(loc));
  }

  PexValue::TemporaryVariable allocTemp(const papyrus::PapyrusType& tp) {
    auto vRef = new PexTemporaryVariableRef(tp.buildPex(file));
    tempVarRefs.push_back(vRef);
    return PexValue::TemporaryVariable(vRef);
  }

  PexFunctionBuilder& operator <<(PexLabel* loc) {
    loc->targetIdx = instructions.size();
    return *this;
  }

  PexLabel* label() {
    PexLabel* l;
    *this >> l;
    return l;
  }

  PexFunctionBuilder& operator >>(PexLabel*& loc) {
    loc = new PexLabel();
    labels.push_back(loc);
    return *this;
  }

  PexLabel* currentBreakTarget() {
    return curBreakStack.back();
  }

  void pushBreakScope(PexLabel* destLabel) {
    curBreakStack.push_back(destLabel);
  }

  void popBreakScope() {
    curBreakStack.pop_back();
  }

  PexLabel* currentContinueTarget() {
    return curContinueStack.back();
  }

  void pushBreakContinueScope(PexLabel* breakLabel, PexLabel* continueLabel) {
    curBreakStack.push_back(breakLabel);
    curContinueStack.push_back(continueLabel);
  }

  void popBreakContinueScope() {
    curBreakStack.pop_back();
    curContinueStack.pop_back();
  }


  void freeValueIfTemp(const PexValue& v);
  void populateFunction(PexFunction* func, PexDebugFunctionInfo* debInfo);

  PexFunctionBuilder(const CapricaFileLocation& loc, PexFile* fl) : currentLocation(loc), file(fl) { }

private:
  PexFile* file;
  CapricaFileLocation currentLocation;
  std::vector<CapricaFileLocation> instructionLocations{ };
  std::vector<PexInstruction*> instructions{ };
  std::vector<PexLocalVariable*> locals{ };
  std::vector<PexLabel*> labels{ };
  std::vector<PexTemporaryVariableRef*> tempVarRefs{ };
  std::map<PexString, PexLocalVariable*> tempVarNameTypeMap{ };
  std::set<PexString> longLivedTempVars{ };
  std::map<PexString, std::vector<PexLocalVariable*>> freeTempVars{ };
  size_t currentTempI = 0;
  std::vector<PexLabel*> curBreakStack{ };
  std::vector<PexLabel*> curContinueStack{ };

  template<typename... Args>
  PexFunctionBuilder& push(PexOpCode op, Args&&... args) {
    return push(new PexInstruction(op, std::vector<PexValue>{ args... }));
  }
  
  PexFunctionBuilder& push(PexInstruction* instr);
  PexLocalVariable* internalAllocateTempVar(const PexString& typeName);
};

}}
