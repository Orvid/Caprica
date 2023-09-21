#pragma once

#include <common/identifier_ref.h>

#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusDeclareStatement final : public PapyrusStatement
{
  PapyrusType type;
  identifier_ref name{ "" };
  bool isAuto{ false };
  bool isConst{ false };
  expressions::PapyrusExpression* initialValue{ nullptr };

  explicit PapyrusDeclareStatement(CapricaFileLocation loc, PapyrusType&& tp) : PapyrusStatement(loc), type(std::move(tp)) { }
  PapyrusDeclareStatement(const PapyrusDeclareStatement&) = delete;
  virtual ~PapyrusDeclareStatement() override {
    if (initialValue)
      delete initialValue;
  }

  virtual bool buildCFG(PapyrusCFG& cfg) const override {
    cfg.appendStatement(this);
    return false;
  }

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    auto loc = bldr.allocateLocal(name, type);
    if (initialValue) {
      auto val = initialValue->generateLoad(file, bldr);
      bldr << location;
      bldr << op::assign{ loc, val };
    }
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    if (isAuto) {
      initialValue->semantic(ctx);
      ctx->checkForPoison(initialValue);
      type = initialValue->resultType();
    } else {
      type = ctx->resolveType(type);

      if (initialValue) {
        initialValue->semantic(ctx);
        ctx->checkForPoison(initialValue);
        initialValue = ctx->coerceExpression(initialValue, type);
      }
    }

    ctx->addLocalVariable(this);
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);
  }
};

}}}
