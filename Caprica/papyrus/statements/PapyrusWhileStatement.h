#pragma once

#include <vector>

#include <common/CapricaFileLocation.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusWhileStatement final : public PapyrusStatement
{
  expressions::PapyrusExpression* condition{ nullptr };
  std::vector<PapyrusStatement*> body{ };

  PapyrusWhileStatement(const CapricaFileLocation& loc) : PapyrusStatement(loc) { }
  virtual ~PapyrusWhileStatement() override {
    if (condition)
      delete condition;
    for (auto s : body)
      delete s;
  }

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    pex::PexLabel* beforeCondition;
    bldr >> beforeCondition;
    bldr << beforeCondition;
    pex::PexLabel* afterAll;
    bldr >> afterAll;
    auto lVal = condition->generateLoad(file, bldr);
    bldr << location;
    bldr << op::jmpf{ lVal, afterAll };

    for (auto s : body)
      s->buildPex(file, bldr);
    bldr << location;
    bldr << op::jmp{ beforeCondition };

    bldr << afterAll;
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    condition->semantic(ctx);
    condition = PapyrusResolutionContext::coerceExpression(condition, PapyrusType::Bool(condition->location));
    ctx->pushIdentifierScope();
    for (auto s : body)
      s->semantic(ctx);
    ctx->popIdentifierScope();
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);

    for (auto s : body)
      s->visit(visitor);
  }
};

}}}
