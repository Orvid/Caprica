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
    namespace op = caprica::pex::op;
    auto iVal = innerExpression->generateLoad(file, bldr);
    auto dest = bldr.allocTemp(file, this->resultType());
    bldr << location;
    switch (operation) {
      case PapyrusUnaryOperatorType::Negate:
        if (innerExpression->resultType() == PapyrusType::Float())
          bldr << op::fneg{ dest, iVal };
        else if (innerExpression->resultType() == PapyrusType::Int())
          bldr << op::ineg{ dest, iVal };
        else
          throw std::runtime_error("You can only negate integers and floats!");
        break;
      case PapyrusUnaryOperatorType::Not:
        bldr << op::not{ dest, iVal };
        break;
      default:
        throw std::runtime_error("Unknown PapyrusBinaryOperatorType while generating the pex opcodes!");
    }
    bldr.freeIfTemp(iVal);
    return dest;
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
