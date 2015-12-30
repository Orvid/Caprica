#pragma once

#include <papyrus/expressions/PapyrusBinaryOpExpression.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/expressions/PapyrusIdentifierExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

enum class PapyrusAssignOperatorType
{
  None,

  Assign,
  Add,
  Subtract,
  Multiply,
  Divide,
  Modulus,
};

struct PapyrusAssignStatement final : public PapyrusStatement
{
  expressions::PapyrusExpression* lValue{ nullptr };
  PapyrusAssignOperatorType operation{ PapyrusAssignOperatorType::None };
  expressions::PapyrusExpression* rValue{ nullptr };
  expressions::PapyrusBinaryOpExpression* binOpExpression{ nullptr };

  PapyrusAssignStatement(parser::PapyrusFileLocation loc) : PapyrusStatement(loc) { }
  ~PapyrusAssignStatement() {
    // This is a bit of a special case, because we still need the lValue, but
    // we've given ownership of the lValue to the binOpExpression, to avoid
    // having to adjust the binOpExpression for this single use-case.
    if (binOpExpression) {
      if (rValue != binOpExpression->right)
        delete rValue;
      else
        delete binOpExpression;
    } else {
      if (lValue)
        delete lValue;
      if (rValue)
        delete rValue;
    }
  }

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    pex::PexValue rVal;
    if (binOpExpression)
      rVal = binOpExpression->generateLoad(file, bldr);
    else
      rVal = rValue->generateLoad(file, bldr);

    if (auto id = lValue->as<expressions::PapyrusIdentifierExpression>()) {
      bldr << location;
      id->identifier.generateStore(file, bldr, pex::PexValue::Identifier(file->getString("self")), rVal);
    } else if (auto ai = lValue->as<expressions::PapyrusArrayIndexExpression>()) {
      bldr << location;
      ai->generateStore(file, bldr, rVal);
    } else {
      throw std::runtime_error("Invalid Lefthand Side for PapyrusAssignStatement!");
    }

    bldr.freeIfTemp(rVal);
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    if (operation == PapyrusAssignOperatorType::Assign) {
      lValue->semantic(ctx);
      rValue->semantic(ctx);
      rValue = expressions::PapyrusExpression::coerceExpression(rValue, lValue->resultType());
    } else {
      binOpExpression = new expressions::PapyrusBinaryOpExpression(location);
      binOpExpression->left = lValue;
      binOpExpression->right = rValue;
      switch (operation) {
        case PapyrusAssignOperatorType::Add:
          binOpExpression->operation = expressions::PapyrusBinaryOperatorType::Add;
          break;
        case PapyrusAssignOperatorType::Subtract:
          binOpExpression->operation = expressions::PapyrusBinaryOperatorType::Subtract;
          break;
        case PapyrusAssignOperatorType::Multiply:
          binOpExpression->operation = expressions::PapyrusBinaryOperatorType::Multiply;
          break;
        case PapyrusAssignOperatorType::Divide:
          binOpExpression->operation = expressions::PapyrusBinaryOperatorType::Divide;
          break;
        case PapyrusAssignOperatorType::Modulus:
          binOpExpression->operation = expressions::PapyrusBinaryOperatorType::Modulus;
          break;
        default:
          throw std::runtime_error("Unknown PapyrusAssignOperatorType!");
      }
      binOpExpression->semantic(ctx);
      rValue = expressions::PapyrusExpression::coerceExpression(binOpExpression, lValue->resultType());
    }
    if (auto id = lValue->as<expressions::PapyrusIdentifierExpression>()) {
      if (id->identifier.type == PapyrusIdentifierType::Property && id->identifier.prop->isReadOnly)
        ctx->fatalError("Attempted to assign to a read-only property!");
    } else if (auto ai = lValue->as<expressions::PapyrusArrayIndexExpression>()) {
      // It's valid.
    } else {
      throw std::runtime_error("Invalid Lefthand Side for PapyrusAssignStatement!");
    }
  }
};

}}}
