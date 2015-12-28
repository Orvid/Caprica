#pragma once

#include <vector>

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusWhileStatement final : public PapyrusStatement
{
  expressions::PapyrusExpression* condition{ nullptr };
  std::vector<PapyrusStatement*> body{ };

  PapyrusWhileStatement(parser::PapyrusFileLocation loc) : PapyrusStatement(loc) { }
  ~PapyrusWhileStatement() {
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
    condition = expressions::PapyrusExpression::coerceExpression(condition, PapyrusType::Bool());
    for (auto s : body)
      s->semantic(ctx);
  }
};

}}}
