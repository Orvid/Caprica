#pragma once

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/PapyrusObject.h>
#include <papyrus/PapyrusType.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusArrayIndexExpression final : public PapyrusExpression {
  PapyrusExpression* baseExpression { nullptr };
  PapyrusExpression* indexExpression { nullptr };

  explicit PapyrusArrayIndexExpression(CapricaFileLocation loc) : PapyrusExpression(loc) { }
  PapyrusArrayIndexExpression(const PapyrusArrayIndexExpression&) = delete;
  virtual ~PapyrusArrayIndexExpression() override = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    auto base = baseExpression->generateLoad(file, bldr);
    auto idx = indexExpression->generateLoad(file, bldr);
    bldr << location;
    auto dest = bldr.allocTemp(this->resultType());
    bldr << op::arraygetelement { dest, pex::PexValue::Identifier::fromVar(base), idx };
    return dest;
  }

  void generateStore(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue val) const {
    namespace op = caprica::pex::op;
    auto base = baseExpression->generateLoad(file, bldr);
    auto idx = indexExpression->generateLoad(file, bldr);
    bldr << location;
    bldr << op::arraysetelement { pex::PexValue::Identifier::fromVar(base), idx, val };
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    baseExpression->semantic(ctx);
    ctx->checkForPoison(baseExpression);
    if (baseExpression->resultType().type != PapyrusType::Kind::Array) {
      ctx->reportingContext.error(baseExpression->location,
                                  "You can only index arrays! Got '{}'!",
                                  baseExpression->resultType());
    }
    indexExpression->semantic(ctx);
    ctx->checkForPoison(indexExpression);
    indexExpression = ctx->coerceExpression(indexExpression, PapyrusType::Int(indexExpression->location));
  }

  virtual PapyrusType resultType() const override {
    auto res = baseExpression->resultType();
    if (res.type == PapyrusType::Kind::Array)
      return res.getElementType();
    return PapyrusType::None(location);
  }

  virtual PapyrusArrayIndexExpression* asArrayIndexExpression() override { return this; }
};

}}}
