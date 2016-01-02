#pragma once

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusExpressionStatement final : public PapyrusStatement
{
  expressions::PapyrusExpression* expression{ nullptr };

  PapyrusExpressionStatement(const parser::PapyrusFileLocation& loc) : PapyrusStatement(loc) { }
  virtual ~PapyrusExpressionStatement() override {
    if (expression)
      delete expression;
  }

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    bldr.freeIfTemp(expression->generateLoad(file, bldr));
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    expression->semantic(ctx);
  }
};

}}}
