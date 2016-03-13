#include <pex/PexOptimizer.h>

#include <boost/range/adaptor/reversed.hpp>

namespace caprica { namespace pex { 

struct OptInstruction final
{
  size_t id{ 0 };
  size_t instructionNum{ 0 };
  PexInstruction* instr{ nullptr };
  std::vector<OptInstruction*> instructionsReferencingLabel{ };
  OptInstruction* branchTarget{ nullptr };
  uint16_t lineNumber{ 0 };

  explicit OptInstruction(size_t id, PexInstruction* instr) : id(id), instr(instr) { }
  ~OptInstruction() = default;

  void killInstruction() {
    delete instr;
    instr = nullptr;
    branchTarget = nullptr;
    lineNumber = 0;
  }

private:
  explicit OptInstruction() = default;
};

static std::vector<OptInstruction*> buildOptInstructions(const PexDebugFunctionInfo* debInfo,
                                                         const std::vector<PexInstruction*>& instructions) {
  std::unordered_map<size_t, OptInstruction*> labelMap;
  for (size_t i = 0; i < instructions.size(); i++) {
    if (instructions[i]->isBranch()) {
      auto targI = instructions[i]->branchTarget() + i;
      if (!labelMap.count((size_t)(targI)))
        labelMap.insert({ (size_t)targI, new OptInstruction(0, nullptr) });
    }
  }

  std::vector<OptInstruction*> optimizedInstructions{ };
  for (size_t i = 0, id = 0; i < instructions.size(); i++, id++) {
    auto f = labelMap.find(i);
    if (f != labelMap.end()) {
      f->second->id = id++;
      optimizedInstructions.push_back(f->second);
    }

    auto o = new OptInstruction(id, instructions[i]);
    if (debInfo && i < debInfo->instructionLineMap.size())
      o->lineNumber = debInfo->instructionLineMap[i];
    if (o->instr->isBranch()) {
      o->branchTarget = labelMap[(size_t)(o->instr->branchTarget() + i)];
      o->branchTarget->instructionsReferencingLabel.push_back(o);
    }
    optimizedInstructions.push_back(o);

  }

  auto f = labelMap.find(instructions.size());
  if (f != labelMap.end()) {
    f->second->id = optimizedInstructions.size();
    optimizedInstructions.push_back(f->second);
  }
  return optimizedInstructions;
}

void PexOptimizer::optimize(PexFile* file,
                            PexObject* object,
                            PexState* state,
                            PexFunction* function,
                            const std::string& propertyName,
                            PexDebugFunctionType functionType) {
  PexDebugFunctionInfo* debInfo = file->tryFindFunctionDebugInfo(object, state, function, propertyName, functionType);
  auto optimizedInstructions = buildOptInstructions(debInfo, function->instructions);

  const auto isDeadBetween = [&optimizedInstructions](size_t startID, size_t endID) -> bool {
    for (size_t i = startID + 1; i <= endID; i++) {
      assert(i < optimizedInstructions.size());
      if (optimizedInstructions[i]->instr && optimizedInstructions[i]->instr->opCode != PexOpCode::Nop)
        return false;
    }
    return true;
  };

  for (auto& i : boost::adaptors::reverse(optimizedInstructions)) {
    if (i->instr) {
      switch (i->instr->opCode) {
        case PexOpCode::Assign:
          if (i->instr->args[0] == i->instr->args[1])
            i->killInstruction();
          break;
        case PexOpCode::Jmp:
          if (i->id < i->branchTarget->id && isDeadBetween(i->id, i->branchTarget->id))
            i->killInstruction();
          break;
        default:
          break;
      }
    }
  }

  size_t curInstrNum = 0;
  for (auto& i : optimizedInstructions) {
    i->instructionNum = curInstrNum;
    if (i->instr)
      curInstrNum++;
  }

  std::vector<PexInstruction*> newInstructions{ };
  newInstructions.reserve(curInstrNum);
  std::vector<uint16_t> newLineInfo{ };
  newLineInfo.reserve(curInstrNum);
  for (auto& i : optimizedInstructions) {
    if (i->instr) {
      newLineInfo.push_back(i->lineNumber);
      newInstructions.push_back(i->instr);
      if (i->instr->isBranch())
        i->instr->setBranchTarget((int)i->branchTarget->instructionNum - (int)i->instructionNum);
    }
  }

  function->instructions = newInstructions;
  if (debInfo)
    debInfo->instructionLineMap = newLineInfo;

  for (auto& i : optimizedInstructions)
    delete i;
}

}}
