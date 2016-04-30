#pragma once

#include <papyrus/expressions/PapyrusExpression.h>

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

  explicit PapyrusUnaryOpExpression(CapricaFileLocation loc) : PapyrusExpression(loc) { }
  PapyrusUnaryOpExpression(const PapyrusUnaryOpExpression&) = delete;
  virtual ~PapyrusUnaryOpExpression() override {
    if (innerExpression)
      delete innerExpression;
  }

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    auto iVal = innerExpression->generateLoad(file, bldr);
    auto dest = bldr.allocTemp(this->resultType());
    bldr << location;
    switch (operation) {
      case PapyrusUnaryOperatorType::Negate:
        if (innerExpression->resultType().type == PapyrusType::Kind::Float)
          bldr << op::fneg{ dest, iVal };
        else if (innerExpression->resultType().type == PapyrusType::Kind::Int)
          bldr << op::ineg{ dest, iVal };
        else
          bldr.reportingContext.fatal(location, "You can only negate integers and floats!");
        return dest;
      case PapyrusUnaryOperatorType::Not:
        bldr << op::not{ dest, iVal };
        return dest;
      
      case PapyrusUnaryOperatorType::None:
        break;
    }
    CapricaReportingContext::logicalFatal("Unknown PapyrusBinaryOperatorType while generating the pex opcodes!");
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    assert(operation != PapyrusUnaryOperatorType::None);
    innerExpression->semantic(ctx);
    ctx->checkForPoison(innerExpression);
  }

  virtual PapyrusType resultType() const override {
    if (operation == PapyrusUnaryOperatorType::Not)
      return PapyrusType::Bool(location);
    return innerExpression->resultType();
  }
};

}}}
