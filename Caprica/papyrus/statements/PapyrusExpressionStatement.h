#pragma once

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusExpressionStatement final : public PapyrusStatement
{
  expressions::PapyrusExpression* expression{ nullptr };

  explicit PapyrusExpressionStatement(CapricaFileLocation loc) : PapyrusStatement(loc) { }
  PapyrusExpressionStatement(const PapyrusExpressionStatement&) = delete;
  virtual ~PapyrusExpressionStatement() override = default;

  virtual bool buildCFG(PapyrusCFG& cfg) const override {
    cfg.appendStatement(this);
    return false;
  }

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    bldr.freeValueIfTemp(expression->generateLoad(file, bldr));
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    // We don't explicitly use the result of the expression, so we don't
    // check it for poison.
    expression->semantic(ctx);
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);
  }
};

}}}
