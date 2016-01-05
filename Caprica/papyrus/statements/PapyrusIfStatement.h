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

  PapyrusIfStatement(const CapricaFileLocation& loc) : PapyrusStatement(loc) { }
  virtual ~PapyrusIfStatement() override {
    for (auto& i : ifBodies) {
      delete i.first;
      for (auto s : i.second)
        delete s;
    }
    for (auto s : elseStatements)
      delete s;
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
      bldr.freeIfTemp(lVal);
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
      i.first = expressions::PapyrusExpression::coerceExpression(i.first, PapyrusType::Bool(i.first->location));
      ctx->pushIdentifierScope();
      for (auto s : i.second)
        s->semantic(ctx);
      ctx->popIdentifierScope();
    }
    ctx->pushIdentifierScope();
    for (auto s : elseStatements)
      s->semantic(ctx);
    ctx->popIdentifierScope();
  }
};

}}}
