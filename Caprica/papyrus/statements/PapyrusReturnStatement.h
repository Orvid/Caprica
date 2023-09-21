#pragma once

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusReturnStatement final : public PapyrusStatement
{
  expressions::PapyrusExpression* returnValue{ nullptr };

  explicit PapyrusReturnStatement(CapricaFileLocation loc) : PapyrusStatement(loc) { }
  PapyrusReturnStatement(const PapyrusReturnStatement&) = delete;
  virtual ~PapyrusReturnStatement() override = default;

  virtual bool buildCFG(PapyrusCFG& cfg) const override {
    cfg.terminateNode(PapyrusControlFlowNodeEdgeType::Return);
    return true;
  }

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    bldr.generateUnlockGuardsForCurrentLockScope();
    if (!returnValue) {
      bldr << location;
      bldr << op::ret{ pex::PexValue::None() };
    } else {
      auto val = returnValue->generateLoad(file, bldr);
      bldr << location;
      bldr << op::ret{ val };
    }
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    if (returnValue) {
      returnValue->semantic(ctx);
      ctx->checkForPoison(returnValue);
      returnValue = ctx->coerceExpression(returnValue, ctx->function->returnType);
    }
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);
  }
};

}}}
