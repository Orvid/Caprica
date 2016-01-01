#pragma once

#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/parser/PapyrusFileLocation.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusNewArrayExpression final : public PapyrusExpression
{
  PapyrusType type{ };
  PapyrusExpression* lengthExpression{ nullptr };

  PapyrusNewArrayExpression(parser::PapyrusFileLocation loc) : PapyrusExpression(loc) { }
  virtual ~PapyrusNewArrayExpression() override {
    if (lengthExpression)
      delete lengthExpression;
  }

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    auto len = lengthExpression->generateLoad(file, bldr);
    bldr << location;
    auto dest = bldr.allocTemp(file, type);
    bldr << op::arraycreate{ dest, len };
    bldr.freeIfTemp(len);
    return dest;
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    type = ctx->resolveType(type);
    type = PapyrusType::Array(std::make_shared<PapyrusType>(type));
    lengthExpression->semantic(ctx);
  }

  virtual PapyrusType resultType() const override {
    return type;
  }
};

}}}
