#include <pex/PexFunctionBuilder.h>

#include <common/CapricaReportingContext.h>
#include <common/allocators/CachePool.h>

namespace caprica { namespace pex {

static thread_local allocators::CachePool<FixedPexStringMap<detail::TempVarDescriptor>> stringMapCache{ };
PexFunctionBuilder::PexFunctionBuilder(CapricaReportingContext& repCtx, CapricaFileLocation loc, PexFile* fl)
  : reportingContext(repCtx), currentLocation(loc), file(fl), alloc(fl->alloc) {
  tempVarMap = stringMapCache.acquire();
}

void PexFunctionBuilder::populateFunction(PexFunction* func, PexDebugFunctionInfo* debInfo) {
  for (auto cur = instructions.begin(), end = instructions.end(); cur != end; ++cur) {
    for (auto& arg : cur->args) {
      if (arg.type == PexValueType::Label) {
        if (arg.val.l->targetIdx == (size_t)-1)
          CapricaReportingContext::logicalFatal("Unresolved label!");
        auto newVal = arg.val.l->targetIdx - cur.index;
        arg.type = PexValueType::Integer;
        arg.val.i = (int32_t)newVal;
      }
    }
  }

  for (auto l : labels) {
    if (l->targetIdx == (size_t)-1)
      CapricaReportingContext::logicalFatal("Unused unresolved label!");
  }

  for (auto tmp : tempVarRefs) {
    if (tmp->var == nullptr)
      CapricaReportingContext::logicalFatal("Unresolved tmp var!");
  }

  func->instructions = std::move(instructions);
  func->locals = std::move(locals);
  debInfo->instructionLineMap.reserve(func->instructions.size());
  size_t line = 0;
  for (auto l : instructionLocations) {
    line = reportingContext.getLocationLine(l, line);
    if (line > std::numeric_limits<uint16_t>::max())
      reportingContext.fatal(l, "The file has too many lines for the debug info to be able to map correctly!");
    debInfo->instructionLineMap.emplace_back((uint16_t)line);
  }

  stringMapCache.release(tempVarMap);
  tempVarMap = nullptr;
}

void PexFunctionBuilder::freeValueIfTemp(const PexValue& v) {
  PexString varName;
  if (v.type == PexValueType::Identifier)
    varName = v.val.s;
  else if (v.type == PexValueType::TemporaryVar && v.val.tmpVar->var)
    varName = v.val.tmpVar->var->name;
  else
    return;

  detail::TempVarDescriptor* desc;
  if (tempVarMap->tryFind(varName, desc)) {
    if (!desc->isLongLivedTempVar && desc->localVar)
      tempVarMap->findOrCreate(desc->localVar->type)->freeVars.push(desc->localVar);
  }
}

PexLocalVariable* PexFunctionBuilder::internalAllocateTempVar(const PexString& typeName) {
  detail::TempVarDescriptor* desc;
  if (tempVarMap->tryFind(typeName, desc)) {
    if (desc->freeVars.size()) {
      return desc->freeVars.pop();
    }
  }

  constexpr size_t PrefixLength = 6;
  char buf[PrefixLength + 5 + 1] = {
    ':', ':', 't', 'e', 'm', 'p', // strlen() == PrefixLength
    '\0', '\0', '\0', '\0', '\0', // strlen(UINT16_MAX)
    '\0' // NUL
  };
  if (currentTempI > std::numeric_limits<uint16_t>::max())
    CapricaReportingContext::logicalFatal("Exceeded the maximum number of temp vars possible in a function!");
  if (currentTempI > std::numeric_limits<int>::max())
    CapricaReportingContext::logicalFatal("Exceeded the maximum number of temp vars possible in a function!");
  if (_itoa_s((int)currentTempI, buf + PrefixLength, sizeof(buf) - PrefixLength, 10) != 0)
    CapricaReportingContext::logicalFatal("Failed to convert the current temp var index to a string!");
  currentTempI++;

  auto loc = alloc->make<PexLocalVariable>();
  loc->name = file->getString(buf);
  loc->type = typeName;
  tempVarMap->findOrCreate(loc->name)->localVar = loc;
  locals.push_back(loc);
  return loc;
}

PexFunctionBuilder& PexFunctionBuilder::fixup(PexInstruction* instr) {
  for (auto& v : instr->args) {
    if (v.type == PexValueType::Invalid)
      reportingContext.fatal(currentLocation, "Attempted to use an invalid value as a value! (perhaps you tried to use the return value of a function that doesn't return?)");
    if (v.type == PexValueType::TemporaryVar && v.val.tmpVar->var)
      v = PexValue::Identifier(v.val.tmpVar->var);
    freeValueIfTemp(v);
  }
  for (auto& v : instr->variadicArgs) {
    if (v->type == PexValueType::Invalid)
      reportingContext.fatal(currentLocation, "Attempted to use an invalid value as a value! (perhaps you tried to use the return value of a function that doesn't return?)");
    if (v->type == PexValueType::TemporaryVar && v->val.tmpVar->var)
      *v = PexValue::Identifier(v->val.tmpVar->var);
    freeValueIfTemp(*v);
  }

  auto destIdx = PexInstruction::getDestArgIndexForOpCode(instr->opCode);
  if (destIdx != -1 && instr->args[destIdx].type == PexValueType::TemporaryVar) {
    auto loc = internalAllocateTempVar(instr->args[destIdx].val.tmpVar->type);
    instr->args[destIdx].val.tmpVar->var = loc;
    instr->args[destIdx] = PexValue::Identifier(loc);
  }

  for (auto& v : instr->args) {
    if (v.type == PexValueType::TemporaryVar)
      reportingContext.fatal(currentLocation, "Attempted to use a temporary var before it's been assigned!");
  }
  for (auto& v : instr->variadicArgs) {
    if (v->type == PexValueType::TemporaryVar)
      reportingContext.fatal(currentLocation, "Attempted to use a temporary var before it's been assigned!");
  }

  instructionLocations.make<CapricaFileLocation>(currentLocation);
  instructions.push_back(instr);
  return *this;
}

}}
