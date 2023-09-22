#include <pex/PexOptimizer.h>

#include <unordered_map>
#include <vector>

namespace caprica { namespace pex {

struct OptInstruction final {
  size_t id { 0 };
  size_t instructionNum { 0 };
  PexInstruction* instr { nullptr };
  std::vector<OptInstruction*> instructionsReferencingLabel {};
  OptInstruction* branchTarget { nullptr };
  uint16_t lineNumber { 0 };

  explicit OptInstruction(size_t id, PexInstruction* instr) : id(id), instr(instr) { }
  ~OptInstruction() = default;

  bool isDead() const { return instr == nullptr || instr->opCode == PexOpCode::Nop; }

  void killInstruction() {
#if 0
    instr->opCode = PexOpCode::Nop;
    instr->args.clear();
    instr->variadicArgs.clear();
#else
    delete instr;
    instr = nullptr;
    branchTarget = nullptr;
    lineNumber = 0;
#endif
  }

private:
  explicit OptInstruction() = default;
};

static std::vector<OptInstruction*> buildOptInstructions(const PexDebugFunctionInfo* debInfo,
                                                         IntrusiveLinkedList<PexInstruction>& instructions) {
  std::unordered_map<size_t, OptInstruction*> labelMap;
  for (auto cur = instructions.begin(), end = instructions.end(); cur != end; ++cur) {
    if (cur->isBranch()) {
      auto targI = cur->branchTarget() + cur.index;
      if (!labelMap.count((size_t)(targI)))
        labelMap.emplace((size_t)targI, new OptInstruction(0, nullptr));
    }
  }

  std::vector<OptInstruction*> optimizedInstructions {};
  optimizedInstructions.reserve(instructions.size());
  size_t id = 0;
  for (auto cur = instructions.begin(), end = instructions.end(); cur != end; ++cur, id++) {
    auto f = labelMap.find(cur.index);
    if (f != labelMap.end()) {
      f->second->id = id++;
      optimizedInstructions.emplace_back(f->second);
    }

    auto o = new OptInstruction(id, *cur);
    if (debInfo && cur.index < debInfo->instructionLineMap.size())
      o->lineNumber = debInfo->instructionLineMap[cur.index];
    if (o->instr->isBranch()) {
      o->branchTarget = labelMap[(size_t)(o->instr->branchTarget() + cur.index)];
      o->branchTarget->instructionsReferencingLabel.push_back(o);
    }
    optimizedInstructions.emplace_back(o);
  }

  auto f = labelMap.find(instructions.size());
  if (f != labelMap.end()) {
    f->second->id = optimizedInstructions.size();
    optimizedInstructions.emplace_back(f->second);
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
      if (!optimizedInstructions[i]->isDead())
        return false;
    }
    return true;
  };
  const auto nextNonDead = [&optimizedInstructions](size_t startID) -> OptInstruction* {
    for (size_t i = startID + 1; i < optimizedInstructions.size(); i++)
      if (!optimizedInstructions[i]->isDead())
        return optimizedInstructions[i];
    return nullptr;
  };

  for (int i = (int)optimizedInstructions.size() - 1; i >= 0; i--) {
    auto opt = optimizedInstructions[i];
    if (opt->instr) {
      switch (opt->instr->opCode) {
        case PexOpCode::Assign:
          if (opt->instr->args[0] == opt->instr->args[1])
            opt->killInstruction();
          break;
        case PexOpCode::Jmp:
          if (opt->id < opt->branchTarget->id && isDeadBetween(opt->id, opt->branchTarget->id))
            opt->killInstruction();
          break;
#if 0
        case PexOpCode::Not: {
          if (i <= 1)
            break;

          auto n = nextNonDead(opt->id);
          if (!n)
            break;
          if (n->instr->opCode != PexOpCode::JmpF && n->instr->opCode != PexOpCode::JmpT)
            break;
          // Ensure source of branch is dest of not.
          if (opt->instr->args[0] != n->instr->args[0])
            break;

          if (n->instr->opCode == PexOpCode::JmpF)
            n->instr->opCode = PexOpCode::JmpT;
          else if (n->instr->opCode == PexOpCode::JmpT)
            n->instr->opCode = PexOpCode::JmpF;
          else
            CapricaReportingContext::logicalFatal("Somehow got a weird op-code here.");
          n->instr->args[0] = opt->instr->args[1];
          opt->killInstruction();
          break;
        }
#endif
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

  IntrusiveLinkedList<PexInstruction> newInstructions {};
  std::vector<uint16_t> newLineInfo {};
  newLineInfo.reserve(curInstrNum);
  for (auto& i : optimizedInstructions) {
    if (i->instr) {
      newLineInfo.push_back(i->lineNumber);
      newInstructions.push_back(i->instr);
      if (i->instr->isBranch())
        i->instr->setBranchTarget((int)i->branchTarget->instructionNum - (int)i->instructionNum);
    }
  }

  function->instructions = std::move(newInstructions);
  if (debInfo)
    debInfo->instructionLineMap = std::move(newLineInfo);

  for (auto& i : optimizedInstructions)
    delete i;
}

}}
