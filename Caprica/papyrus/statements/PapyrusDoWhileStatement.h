#pragma once

#include <common/IntrusiveLinkedList.h>

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusDoWhileStatement final : public PapyrusStatement
{
  expressions::PapyrusExpression* condition{ nullptr };
  IntrusiveLinkedList<PapyrusStatement> body{ };

  explicit PapyrusDoWhileStatement(CapricaFileLocation loc) : PapyrusStatement(loc) { }
  PapyrusDoWhileStatement(const PapyrusDoWhileStatement&) = delete;
  virtual ~PapyrusDoWhileStatement() override = default;

  virtual bool buildCFG(PapyrusCFG& cfg) const override {
    return cfg.processCommonLoopBody(body);
  }

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    pex::PexLabel* beforeCondition;
    bldr >> beforeCondition;
    pex::PexLabel* afterAll;
    bldr >> afterAll;
    pex::PexLabel* beforeBody;
    bldr >> beforeBody;
    bldr.pushBreakContinueScope(afterAll, beforeCondition);

    bldr << beforeBody;
    for (auto s : body)
      s->buildPex(file, bldr);

    bldr << beforeCondition;
    auto lVal = condition->generateLoad(file, bldr);
    bldr << location;
    bldr << op::jmpt{ lVal, beforeBody };

    bldr << afterAll;
    bldr.popBreakContinueScope();
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    condition->semantic(ctx);
    ctx->checkForPoison(condition);
    condition = ctx->coerceExpression(condition, PapyrusType::Bool(condition->location));
    ctx->pushBreakContinueScope();
    ctx->pushLocalVariableScope();
    for (auto s : body)
      s->semantic(ctx);
    ctx->popLocalVariableScope();
    ctx->popBreakContinueScope();
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);

    for (auto s : body)
      s->visit(visitor);
  }
};

}}}
