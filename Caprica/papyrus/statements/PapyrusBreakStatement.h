#pragma once

#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusBreakStatement final : public PapyrusStatement {
  explicit PapyrusBreakStatement(CapricaFileLocation loc) : PapyrusStatement(loc) { }
  PapyrusBreakStatement(const PapyrusBreakStatement&) = delete;
  virtual ~PapyrusBreakStatement() = default;

  virtual bool buildCFG(PapyrusCFG& cfg) const override {
    cfg.markBreakTerminal();
    cfg.terminateNode(PapyrusControlFlowNodeEdgeType::Break);
    return true;
  }

  virtual void buildPex(pex::PexFile*, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    bldr << op::jmp { bldr.currentBreakTarget() };
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    if (!ctx->canBreak())
      ctx->reportingContext.error(location, "There's nothing to break out of!");
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override { visitor.visit(this); }
};

}}}
