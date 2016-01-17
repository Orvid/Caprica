#pragma once

#include <string>

#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusDeclareStatement final : public PapyrusStatement
{
  PapyrusType type;
  std::string name{ "" };
  bool isAuto{ false };
  expressions::PapyrusExpression* initialValue{ nullptr };

  explicit PapyrusDeclareStatement(const CapricaFileLocation& loc, const PapyrusType& tp) : PapyrusStatement(loc), type(tp) { }
  virtual ~PapyrusDeclareStatement() override {
    if (initialValue)
      delete initialValue;
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
      type = initialValue->resultType();
    } else {
      type = ctx->resolveType(type);

      if (initialValue) {
        initialValue->semantic(ctx);
        initialValue = PapyrusResolutionContext::coerceExpression(initialValue, type);
      }
    }

    ctx->addLocalVariable(this);
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);
  }
};

}}}
