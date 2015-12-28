#pragma once

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/parser/PapyrusFileLocation.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

enum class PapyrusUnaryOperatorType
{
  None,

  Not,
  Negate,
};

struct PapyrusUnaryOpExpression final : public PapyrusExpression
{
  PapyrusUnaryOperatorType operation{ PapyrusUnaryOperatorType::None };
  PapyrusExpression* innerExpression{ nullptr };

  PapyrusUnaryOpExpression(parser::PapyrusFileLocation loc) : PapyrusExpression(loc) { }
  ~PapyrusUnaryOpExpression() {
    if (innerExpression)
      delete innerExpression;
  }

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    // TODO: Implement properly.
    bldr << location;
    return innerExpression->generateLoad(file, bldr);
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    assert(operation != PapyrusUnaryOperatorType::None);
    innerExpression->semantic(ctx);
  }

  virtual PapyrusType resultType() const override {
    return innerExpression->resultType();
  }
};

}}}
