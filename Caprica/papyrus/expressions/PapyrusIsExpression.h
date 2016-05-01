#pragma once

#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusIsExpression final : public PapyrusExpression
{
  PapyrusExpression* innerExpression{ nullptr };
  PapyrusType targetType;

  explicit PapyrusIsExpression(CapricaFileLocation loc, PapyrusType&& tp) : PapyrusExpression(loc), targetType(std::move(tp)) { }
  PapyrusIsExpression(const PapyrusIsExpression&) = delete;
  virtual ~PapyrusIsExpression() override {
    if (innerExpression)
      delete innerExpression;
  }

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;

    auto val = innerExpression->generateLoad(file, bldr);
    auto dest = bldr.allocTemp(resultType());
    bldr << location;
    bldr << op::is{ dest, val, targetType.buildPex(file) };
    return dest;
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    innerExpression->semantic(ctx);
    ctx->checkForPoison(innerExpression);
    targetType = ctx->resolveType(targetType);
  }

  virtual PapyrusType resultType() const override {
    return PapyrusType::Bool(location);
  }
};

}}}
