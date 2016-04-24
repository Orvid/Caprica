#pragma once

#include <utility>
#include <vector>

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusIfStatement final : public PapyrusStatement
{
  std::vector<std::pair<expressions::PapyrusExpression*, std::vector<PapyrusStatement*>>> ifBodies{ };
  std::vector<PapyrusStatement*> elseStatements{ };

  explicit PapyrusIfStatement(CapricaFileLocation loc) : PapyrusStatement(loc) { }
  PapyrusIfStatement(const PapyrusIfStatement&) = delete;
  virtual ~PapyrusIfStatement() override {
    for (auto& i : ifBodies) {
      delete i.first;
      for (auto s : i.second)
        delete s;
    }
    for (auto s : elseStatements)
      delete s;
  }

  virtual bool buildCFG(PapyrusCFG& cfg) const override {
    bool isTerminal = true;

    for (auto p : ifBodies) {
      cfg.addLeaf();
      isTerminal = cfg.processStatements(p.second) && isTerminal;
    }

    if (elseStatements.size()) {
      cfg.addLeaf();
      isTerminal = cfg.processStatements(elseStatements) && isTerminal;
    }

    if (isTerminal)
      cfg.terminateNode(PapyrusControlFlowNodeEdgeType::Children);
    else
      cfg.createSibling();
    return isTerminal;
  }

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    pex::PexLabel* afterAll;
    bldr >> afterAll;
    pex::PexLabel* nextCondition{ nullptr };
    for (auto& ifBody : ifBodies) {
      if (nextCondition)
        bldr << nextCondition;
      bldr >> nextCondition;
      auto lVal = ifBody.first->generateLoad(file, bldr);
      bldr << location;
      bldr << op::jmpf{ lVal, nextCondition };
      for (auto s : ifBody.second)
        s->buildPex(file, bldr);
      bldr << location;
      bldr << op::jmp{ afterAll };
    }

    bldr << nextCondition;
    for (auto s : elseStatements)
      s->buildPex(file, bldr);
    bldr << afterAll;
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    for (auto& i : ifBodies) {
      i.first->semantic(ctx);
      i.first = ctx->coerceExpression(i.first, PapyrusType::Bool(i.first->location));
      ctx->pushLocalVariableScope();
      for (auto s : i.second)
        s->semantic(ctx);
      ctx->popLocalVariableScope();
    }
    ctx->pushLocalVariableScope();
    for (auto s : elseStatements)
      s->semantic(ctx);
    ctx->popLocalVariableScope();
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);

    for (auto& i : ifBodies) {
      for (auto s : i.second)
        s->visit(visitor);
    }
    for (auto s : elseStatements)
      s->visit(visitor);
  }
};

}}}
