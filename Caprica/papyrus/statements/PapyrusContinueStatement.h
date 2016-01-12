#pragma once

#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusContinueStatement final : public PapyrusStatement
{
  PapyrusContinueStatement(const CapricaFileLocation& loc) : PapyrusStatement(loc) { }
  virtual ~PapyrusContinueStatement() = default;

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    bldr << op::jmp{ bldr.currentContinueTarget() };
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    if (!ctx->canContinue())
      CapricaError::error(location, "There's nothing to continue!");
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);
  }
};

}}}
