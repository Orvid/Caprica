#pragma once

#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusBreakStatement final : public PapyrusStatement
{
  PapyrusBreakStatement(const CapricaFileLocation& loc) : PapyrusStatement(loc) { }
  virtual ~PapyrusBreakStatement() = default;

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    bldr << op::jmp{ bldr.currentBreakTarget() };
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    if (!ctx->canBreak())
      CapricaError::error(location, "There's nothing to break out of!");
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);
  }
};

}}}
