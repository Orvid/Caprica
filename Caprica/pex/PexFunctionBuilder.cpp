#include <pex/PexFunctionBuilder.h>

#include <common/CapricaReportingContext.h>

namespace caprica { namespace pex {

void PexFunctionBuilder::populateFunction(PexFunction* func, PexDebugFunctionInfo* debInfo) {
  for (auto cur = instructions.begin(), end = instructions.end(); cur != end; ++cur) {
    for (auto& arg : cur->args) {
      if (arg.type == PexValueType::Label) {
        if (arg.l->targetIdx == (size_t)-1)
          CapricaReportingContext::logicalFatal("Unresolved label!");
        auto newVal = arg.l->targetIdx - cur.index;
        arg.type = PexValueType::Integer;
        arg.i = (int32_t)newVal;
      }
    }
  }

  for (auto l : labels) {
    if (l->targetIdx == (size_t)-1)
      CapricaReportingContext::logicalFatal("Unused unresolved label!");
    delete l;
  }

  for (auto tmp : tempVarRefs) {
    if (tmp->var == nullptr)
      CapricaReportingContext::logicalFatal("Unresolved tmp var!");
    delete tmp;
  }

  func->instructions = std::move(instructions);
  func->locals = std::move(locals);
  debInfo->instructionLineMap.reserve(instructionLocations.size());
  size_t line = 0;
  for (auto l : instructionLocations) {
    line = reportingContext.getLocationLine(l, line);
    if (line > std::numeric_limits<uint16_t>::max())
      reportingContext.fatal(l, "The file has too many lines for the debug info to be able to map correctly!");
    debInfo->instructionLineMap.emplace_back((uint16_t)line);
  }
}

void PexFunctionBuilder::freeValueIfTemp(const PexValue& v) {
  PexString varName;
  if (v.type == PexValueType::Identifier)
    varName = v.s;
  else if (v.type == PexValueType::TemporaryVar && v.tmpVar->var)
    varName = v.tmpVar->var->name;
  else
    return;

  auto f = tempVarNameTypeMap.find(varName);
  if (f != tempVarNameTypeMap.end() && !longLivedTempVars.count(varName)) {
    if (!freeTempVars.count(f->second->type))
      freeTempVars.emplace(f->second->type, std::vector<PexLocalVariable*>{ });
    freeTempVars[f->second->type].emplace_back(f->second);
  }
}

PexLocalVariable* PexFunctionBuilder::internalAllocateTempVar(const PexString& typeName) {
  auto f = freeTempVars.find(typeName);
  if (f != freeTempVars.end() && f->second.size()) {
    PexLocalVariable* b = f->second.back();
    f->second.pop_back();
    return b;
  }

  auto loc = new PexLocalVariable();
  loc->name = file->getString("::temp" + std::to_string(currentTempI++));
  loc->type = typeName;
  tempVarNameTypeMap.emplace(loc->name, loc);
  locals.emplace_back(loc);
  return loc;
}

PexFunctionBuilder& PexFunctionBuilder::fixup(PexInstruction& instr) {
  for (auto& v : instr.args) {
    if (v.type == PexValueType::Invalid)
      reportingContext.fatal(currentLocation, "Attempted to use an invalid value as a value! (perhaps you tried to use the return value of a function that doesn't return?)");
    if (v.type == PexValueType::TemporaryVar && v.tmpVar->var)
      v = PexValue::Identifier(v.tmpVar->var);
    freeValueIfTemp(v);
  }
  for (auto& v : instr.variadicArgs) {
    if (v.type == PexValueType::Invalid)
      reportingContext.fatal(currentLocation, "Attempted to use an invalid value as a value! (perhaps you tried to use the return value of a function that doesn't return?)");
    if (v.type == PexValueType::TemporaryVar && v.tmpVar->var)
      v = PexValue::Identifier(v.tmpVar->var);
    freeValueIfTemp(v);
  }

  auto destIdx = PexInstruction::getDestArgIndexForOpCode(instr.opCode);
  if (destIdx != -1 && instr.args[destIdx].type == PexValueType::TemporaryVar) {
    auto loc = internalAllocateTempVar(instr.args[destIdx].tmpVar->type);
    instr.args[destIdx].tmpVar->var = loc;
    instr.args[destIdx] = PexValue::Identifier(loc);
  }

  for (auto& v : instr.args) {
    if (v.type == PexValueType::TemporaryVar)
      reportingContext.fatal(currentLocation, "Attempted to use a temporary var before it's been assigned!");
  }
  for (auto& v : instr.variadicArgs) {
    if (v.type == PexValueType::TemporaryVar)
      reportingContext.fatal(currentLocation, "Attempted to use a temporary var before it's been assigned!");
  }

  instructionLocations.emplace_back(currentLocation);
  return *this;
}

}}
