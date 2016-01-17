#pragma once

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusDoWhileStatement final : public PapyrusStatement
{
  expressions::PapyrusExpression* condition{ nullptr };
  std::vector<PapyrusStatement*> body{ };

  explicit PapyrusDoWhileStatement(const CapricaFileLocation& loc) : PapyrusStatement(loc) { }
  virtual ~PapyrusDoWhileStatement() override {
    if (condition)
      delete condition;
    for (auto s : body)
      delete s;
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
    condition = PapyrusResolutionContext::coerceExpression(condition, PapyrusType::Bool(condition->location));
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
