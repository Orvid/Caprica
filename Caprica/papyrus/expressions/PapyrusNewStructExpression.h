#pragma once

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/PapyrusType.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusNewStructExpression final : public PapyrusExpression {
  PapyrusType type;

  explicit PapyrusNewStructExpression(CapricaFileLocation loc, PapyrusType&& tp)
      : PapyrusExpression(loc), type(std::move(tp)) { }
  PapyrusNewStructExpression(const PapyrusNewStructExpression&) = delete;
  virtual ~PapyrusNewStructExpression() override = default;

  virtual pex::PexValue generateLoad(pex::PexFile*, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    bldr << location;
    auto dest = bldr.allocTemp(type);
    bldr << op::structcreate { dest };
    return dest;
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override { type = ctx->resolveType(type); }

  virtual PapyrusType resultType() const override { return type; }
};

}}}
