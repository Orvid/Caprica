#pragma once

#include <common/EngineLimits.h>

#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/expressions/PapyrusLiteralExpression.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusNewArrayExpression final : public PapyrusExpression
{
  PapyrusType type;
  PapyrusExpression* lengthExpression{ nullptr };

  explicit PapyrusNewArrayExpression(CapricaFileLocation loc, PapyrusType&& tp) : PapyrusExpression(loc), type(std::move(tp)) { }
  PapyrusNewArrayExpression(const PapyrusNewArrayExpression&) = delete;
  virtual ~PapyrusNewArrayExpression() override = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    auto len = lengthExpression->generateLoad(file, bldr);
    bldr << location;
    auto dest = bldr.allocTemp(type);
    bldr << op::arraycreate{ dest, len };
    return dest;
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    type = ctx->resolveType(type);
    type = PapyrusType::Array(type.location, ctx->allocator->make<PapyrusType>(type));
    lengthExpression->semantic(ctx);
    ctx->checkForPoison(lengthExpression);

    if (lengthExpression->resultType().type != PapyrusType::Kind::Int)
      ctx->reportingContext.error(lengthExpression->location, "The length expression of a new array expression must be an integral type, but got '%s'.", lengthExpression->resultType().prettyString().c_str());
    else if (auto len = lengthExpression->asLiteralExpression()) {
      if (len->value.i < 0)
        ctx->reportingContext.error(len->value.location, "The length expression of a new array expression must be greater than or equal to zero. Got '%lli'.", (int64_t)len->value.i);
      else
        EngineLimits::checkLimit(ctx->reportingContext, len->value.location, EngineLimits::Type::ArrayLength, len->value.i);
    }
  }

  virtual PapyrusType resultType() const override {
    return type;
  }
};

}}}
