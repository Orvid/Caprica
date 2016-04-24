#pragma once

#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusContinueStatement final : public PapyrusStatement
{
  explicit PapyrusContinueStatement(CapricaFileLocation loc) : PapyrusStatement(loc) { }
  PapyrusContinueStatement(const PapyrusContinueStatement&) = delete;
  virtual ~PapyrusContinueStatement() = default;

  virtual bool buildCFG(PapyrusCFG& cfg) const override {
    cfg.terminateNode(PapyrusControlFlowNodeEdgeType::Continue);
    return true;
  }

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    bldr << op::jmp{ bldr.currentContinueTarget() };
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    if (!ctx->canContinue())
      ctx->reportingContext.error(location, "There's nothing to continue!");
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);
  }
};

}}}
