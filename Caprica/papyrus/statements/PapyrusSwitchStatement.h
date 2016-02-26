#pragma once

#include <utility>
#include <vector>

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusSwitchStatement final : public PapyrusStatement
{
  expressions::PapyrusExpression* condition{ nullptr };
  std::vector<std::pair<PapyrusValue, std::vector<PapyrusStatement*>>> caseBodies{ };
  std::vector<PapyrusStatement*> defaultStatements{ };

  explicit PapyrusSwitchStatement(const CapricaFileLocation& loc) : PapyrusStatement(loc) { }
  PapyrusSwitchStatement(const PapyrusSwitchStatement&) = delete;
  virtual ~PapyrusSwitchStatement() override {
    if (condition)
      delete condition;
    for (auto& i : caseBodies) {
      for (auto s : i.second)
        delete s;
    }
    for (auto s : defaultStatements)
      delete s;
  }

  virtual bool buildCFG(PapyrusCFG& cfg) const override {
    cfg.pushBreakTerminal();
    for (auto p : caseBodies) {
      cfg.addLeaf();
      if (!cfg.processStatements(p.second))
        CapricaError::error(p.first.location, "Control is not allowed to fall through to the next case.");
    }

    if (defaultStatements.size()) {
      cfg.addLeaf();
      if (!cfg.processStatements(defaultStatements))
        CapricaError::error(defaultStatements[0]->location, "Control is not allowed to fall through to the next case.");
    }

    bool isTerminal = !cfg.popBreakTerminal();
    if (isTerminal)
      cfg.terminateNode(PapyrusControlFlowNodeEdgeType::Children);
    else
      cfg.createSibling();
    return isTerminal;
  }

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;

    auto tmpDest = bldr.allocLongLivedTemp(condition->resultType());
    bldr << location;
    bldr << op::assign{ tmpDest, condition->generateLoad(file, bldr) };

    pex::PexLabel* afterAll;
    bldr >> afterAll;
    bldr.pushBreakScope(afterAll);
    pex::PexLabel* nextCondition{ nullptr };
    for (auto& cBody : caseBodies) {
      if (nextCondition)
        bldr << nextCondition;
      bldr >> nextCondition;
      bldr << location;
      auto cond = bldr.allocTemp(PapyrusType::Bool(location));
      bldr << op::cmpeq{ cond, tmpDest, cBody.first.buildPex(file) };
      bldr << op::jmpf{ cond, nextCondition };
      for (auto s : cBody.second)
        s->buildPex(file, bldr);
    }
    bldr.freeLongLivedTemp(tmpDest);

    bldr << nextCondition;
    for (auto s : defaultStatements)
      s->buildPex(file, bldr);
    bldr.popBreakScope();
    bldr << afterAll;
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    condition->semantic(ctx);
    if (condition->resultType().type != PapyrusType::Kind::Int && condition->resultType().type != PapyrusType::Kind::String)
      CapricaError::error(condition->location, "The condition of a switch statement can only be either an Int or a String, got '%s'!", condition->resultType().prettyString().c_str());

    ctx->pushBreakScope();
    for (auto& i : caseBodies) {
      if (i.first.getPapyrusType() != condition->resultType())
        CapricaError::error(i.first.location, "The condition of a case statement must match the type of the expression being switched on! Expected '%s', got '%s'!", condition->resultType().prettyString().c_str(), i.first.getPapyrusType().prettyString().c_str());
      ctx->pushLocalVariableScope();
      for (auto s : i.second)
        s->semantic(ctx);
      ctx->popLocalVariableScope();
    }
    ctx->pushLocalVariableScope();
    for (auto s : defaultStatements)
      s->semantic(ctx);
    ctx->popLocalVariableScope();
    ctx->popBreakScope();
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);

    for (auto& i : caseBodies) {
      for (auto s : i.second)
        s->visit(visitor);
    }
    for (auto s : defaultStatements)
      s->visit(visitor);
  }
};

}}}
