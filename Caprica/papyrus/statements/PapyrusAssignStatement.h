#pragma once

#include <common/CapricaConfig.h>

#include <papyrus/expressions/PapyrusArrayIndexExpression.h>
#include <papyrus/expressions/PapyrusBinaryOpExpression.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/expressions/PapyrusIdentifierExpression.h>
#include <papyrus/expressions/PapyrusMemberAccessExpression.h>
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

  PapyrusAssignStatement(const CapricaFileLocation& loc) : PapyrusStatement(loc) { }
  virtual ~PapyrusAssignStatement() override {
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
    } else if (auto ma = lValue->as<expressions::PapyrusMemberAccessExpression>()) {
      bldr << location;
      ma->generateStore(file, bldr, rVal);
    } else {
      CapricaError::logicalFatal("Invalid Lefthand Side for PapyrusAssignStatement!");
    }

    bldr.freeIfTemp(rVal);
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    if (operation == PapyrusAssignOperatorType::Assign) {
      lValue->semantic(ctx);
      rValue->semantic(ctx);
      rValue = PapyrusResolutionContext::coerceExpression(rValue, lValue->resultType());
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
          CapricaError::logicalFatal("Unknown PapyrusAssignOperatorType!");
      }
      binOpExpression->semantic(ctx);
      if (lValue->resultType().type == PapyrusType::Kind::Array && !CapricaConfig::enableLanguageExtensions)
        CapricaError::fatal(location, "You can't do anything except assign to an array element unless you have language extensions enabled!");
      rValue = PapyrusResolutionContext::coerceExpression(binOpExpression, lValue->resultType());
    }
    if (auto id = lValue->as<expressions::PapyrusIdentifierExpression>()) {
      if (id->identifier.type == PapyrusIdentifierType::Property && id->identifier.prop->isReadOnly)
        CapricaError::fatal(location, "Attempted to assign to a read-only property!");
    } else if (auto ai = lValue->as<expressions::PapyrusArrayIndexExpression>()) {
      // It's valid.
    } else if (auto ma = lValue->as<expressions::PapyrusMemberAccessExpression>()) {
      // It's valid.
    } else {
      CapricaError::fatal(lValue->location, "Invalid Lefthand Side for PapyrusAssignStatement!");
    }
  }
};

}}}
