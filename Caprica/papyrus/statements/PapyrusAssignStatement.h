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

  explicit PapyrusAssignStatement(CapricaFileLocation loc) : PapyrusStatement(loc) { }
  PapyrusAssignStatement(const PapyrusAssignStatement&) = delete;
  virtual ~PapyrusAssignStatement() override = default;

  virtual bool buildCFG(PapyrusCFG& cfg) const override {
    cfg.appendStatement(this);
    return false;
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
      CapricaReportingContext::logicalFatal("Invalid Lefthand Side for PapyrusAssignStatement!");
    }
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    if (auto id = lValue->as<expressions::PapyrusIdentifierExpression>()) {
      id->isAssignmentContext = true;
    }

    if (operation == PapyrusAssignOperatorType::Assign) {
      lValue->semantic(ctx);
      ctx->checkForPoison(lValue);
      rValue->semantic(ctx);
      ctx->checkForPoison(rValue);
      rValue = ctx->coerceExpression(rValue, lValue->resultType());
    } else {
      binOpExpression = ctx->allocator->make<expressions::PapyrusBinaryOpExpression>(location);
      binOpExpression->left = lValue;
      binOpExpression->right = rValue;
      binOpExpression->operation = [](PapyrusAssignOperatorType op) {
        switch (op) {
          case PapyrusAssignOperatorType::Add:
            return expressions::PapyrusBinaryOperatorType::Add;
          case PapyrusAssignOperatorType::Subtract:
            return expressions::PapyrusBinaryOperatorType::Subtract;
          case PapyrusAssignOperatorType::Multiply:
            return expressions::PapyrusBinaryOperatorType::Multiply;
          case PapyrusAssignOperatorType::Divide:
            return expressions::PapyrusBinaryOperatorType::Divide;
          case PapyrusAssignOperatorType::Modulus:
            return expressions::PapyrusBinaryOperatorType::Modulus;
          case PapyrusAssignOperatorType::Assign:
          case PapyrusAssignOperatorType::None:
            break;
        }
        CapricaReportingContext::logicalFatal("Unknown PapyrusAssignOperatorType!");
      }(operation);
      binOpExpression->semantic(ctx);
      ctx->checkForPoison(binOpExpression);
      if (lValue->resultType().type == PapyrusType::Kind::Array && !conf::Papyrus::enableLanguageExtensions)
        ctx->reportingContext.error(location, "You can't do anything except assign to an array element unless you have language extensions enabled!");
      rValue = ctx->coerceExpression(binOpExpression, lValue->resultType());
    }

    if (auto id = lValue->as<expressions::PapyrusIdentifierExpression>()) {
      id->identifier.ensureAssignable(ctx->reportingContext);
      id->identifier.markWritten();
    } else if (auto ai = lValue->as<expressions::PapyrusArrayIndexExpression>()) {
      // It's valid.
    } else if (auto ma = lValue->as<expressions::PapyrusMemberAccessExpression>()) {
      if (auto id = ma->accessExpression->as<expressions::PapyrusIdentifierExpression>())
        id->identifier.ensureAssignable(ctx->reportingContext);
    } else {
      ctx->reportingContext.fatal(lValue->location, "Invalid Lefthand Side for PapyrusAssignStatement!");
    }
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);
  }
};

}}}
