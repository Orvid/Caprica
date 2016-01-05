#pragma once

#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusNewStructExpression final : public PapyrusExpression
{
  PapyrusType type;

  PapyrusNewStructExpression(const CapricaFileLocation& loc, const PapyrusType& tp) : PapyrusExpression(loc), type(tp) { }
  virtual ~PapyrusNewStructExpression() override = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    bldr << location;
    auto dest = bldr.allocTemp(file, type);
    bldr << op::structcreate{ dest };
    return dest;
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    type = ctx->resolveType(type);
  }

  virtual PapyrusType resultType() const override {
    return type;
  }
};

}}}
