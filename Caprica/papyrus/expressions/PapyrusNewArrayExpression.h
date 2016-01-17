#pragma once

#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusNewArrayExpression final : public PapyrusExpression
{
  PapyrusType type;
  PapyrusExpression* lengthExpression{ nullptr };

  explicit PapyrusNewArrayExpression(const CapricaFileLocation& loc, const PapyrusType& tp) : PapyrusExpression(loc), type(tp) { }
  virtual ~PapyrusNewArrayExpression() override {
    if (lengthExpression)
      delete lengthExpression;
  }

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
    type = PapyrusType::Array(type.location, std::make_shared<PapyrusType>(type));
    lengthExpression->semantic(ctx);
  }

  virtual PapyrusType resultType() const override {
    return type;
  }
};

}}}
