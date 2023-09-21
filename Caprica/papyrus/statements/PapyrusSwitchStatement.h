#pragma once

#include <common/IntrusiveLinkedList.h>

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusSwitchStatement final : public PapyrusStatement
{
  struct CaseBody final
  {
    PapyrusValue condition;
    IntrusiveLinkedList<PapyrusStatement> body{ };

    CaseBody(PapyrusValue&& c, IntrusiveLinkedList<PapyrusStatement>&& b) : condition(std::move(c)), body(std::move(b)) { }

  private:
    friend IntrusiveLinkedList<CaseBody>;
    CaseBody* next{ nullptr };
  };
  expressions::PapyrusExpression* condition{ nullptr };
  IntrusiveLinkedList<CaseBody> caseBodies{ };
  IntrusiveLinkedList<PapyrusStatement> defaultStatements{ };

  explicit PapyrusSwitchStatement(CapricaFileLocation loc) : PapyrusStatement(loc) { }
  PapyrusSwitchStatement(const PapyrusSwitchStatement&) = delete;
  virtual ~PapyrusSwitchStatement() override = default;

  virtual bool buildCFG(PapyrusCFG& cfg) const override {
    cfg.pushBreakTerminal();
    for (auto p : caseBodies) {
      cfg.addLeaf();
      if (!cfg.processStatements(p->body))
        cfg.reportingContext.error(p->condition.location, "Control is not allowed to fall through to the next case.");
    }

    if (defaultStatements.size()) {
      cfg.addLeaf();
      if (!cfg.processStatements(defaultStatements))
        cfg.reportingContext.error(defaultStatements.front()->location, "Control is not allowed to fall through to the next case.");
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
      bldr << op::cmpeq{ cond, tmpDest, cBody->condition.buildPex(file) };
      bldr << op::jmpf{ cond, nextCondition };
      for (auto s : cBody->body)
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
    ctx->checkForPoison(condition);
    if (condition->resultType().type != PapyrusType::Kind::Int && condition->resultType().type != PapyrusType::Kind::String)
      ctx->reportingContext.error(condition->location, "The condition of a switch statement can only be either an Int or a String, got '%s'!", condition->resultType().prettyString().c_str());

    ctx->pushBreakScope();
    for (auto i : caseBodies) {
      if (i->condition.getPapyrusType() != condition->resultType())
        ctx->reportingContext.error(i->condition.location, "The condition of a case statement must match the type of the expression being switched on! Expected '%s', got '%s'!", condition->resultType().prettyString().c_str(), i->condition.getPapyrusType().prettyString().c_str());
      ctx->pushLocalVariableScope();
      for (auto s : i->body)
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

    for (auto i : caseBodies) {
      for (auto s : i->body)
        s->visit(visitor);
    }
    for (auto s : defaultStatements)
      s->visit(visitor);
  }
};

}}}
