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

enum class PapyrusAssignOperatorType {
  None,

  Assign,
  Add,
  Subtract,
  Multiply,
  Divide,
  Modulus,
};

struct PapyrusAssignStatement final : public PapyrusStatement {
private:
  struct CapturedExpression final : public expressions::PapyrusExpression {
    expressions::PapyrusExpression* expression;
    pex::PexLocalVariable* local { nullptr };

    explicit CapturedExpression(expressions::PapyrusExpression* expr)
        : PapyrusExpression(expr->location), expression(expr) { }
    CapturedExpression(const CapturedExpression&) = delete;
    virtual ~CapturedExpression() override = default;

    void capture(pex::PexFile* file, pex::PexFunctionBuilder& bldr) {
      auto value = expression->generateLoad(file, bldr);
      local = bldr.allocLongLivedTemp(resultType());
      bldr << location;
      bldr << pex::op::assign { local, value };
    }

    virtual pex::PexValue generateLoad(pex::PexFile*, pex::PexFunctionBuilder&) const override { return local; }
    virtual void semantic(PapyrusResolutionContext* ctx) override { expression->semantic(ctx); }
    virtual PapyrusType resultType() const override { return expression->resultType(); }
  };

  CapturedExpression* capturedBase { nullptr };
  CapturedExpression* capturedIndex { nullptr };

public:
  expressions::PapyrusExpression* lValue { nullptr };
  PapyrusAssignOperatorType operation { PapyrusAssignOperatorType::None };
  expressions::PapyrusExpression* rValue { nullptr };
  expressions::PapyrusBinaryOpExpression* binOpExpression { nullptr };

  explicit PapyrusAssignStatement(CapricaFileLocation loc) : PapyrusStatement(loc) { }
  PapyrusAssignStatement(const PapyrusAssignStatement&) = delete;
  virtual ~PapyrusAssignStatement() override = default;

  virtual bool buildCFG(PapyrusCFG& cfg) const override {
    cfg.appendStatement(this);
    return false;
  }

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    if (capturedBase)
      capturedBase->capture(file, bldr);
    if (capturedIndex)
      capturedIndex->capture(file, bldr);
    pex::PexValue rVal;
    if (binOpExpression) {
      rVal = binOpExpression->generateLoad(file, bldr);
    } else {
      rVal = rValue->generateLoad(file, bldr);
      if (conf::Papyrus::game == GameID::Skyrim && rVal.type == pex::PexValueType::Invalid) {
        // check if if the rValue is the result of a function call
        auto travRValueExpression = rValue;
        while (true) {
          if (auto fc = travRValueExpression->asFunctionCallExpression()) {
            // this was a void method call that was assigned to this variable; change it to None and emit a warning
            if (fc->resultType().type == PapyrusType::Kind::None) {
              rVal = bldr.getNoneLocal(fc->location);
              bldr.reportingContext.warning_W7004_Skyrim_Assignment_Of_Void_Call_Result(
                  fc->location,
                  fc->function.res.name);
            }
            break;
          }
          if (auto ma = travRValueExpression->asMemberAccessExpression()) {
            travRValueExpression = ma->accessExpression;
            continue;
          }
          if (auto ai = travRValueExpression->asCastExpression()) {
            travRValueExpression = ai->innerExpression;
            continue;
          }
          break; // we didn't find a function call expression
        }
      }
    }
    if (auto id = lValue->asIdentifierExpression()) {
      bldr << location;
      id->identifier.generateStore(file, bldr, pex::PexValue::Identifier(file->getString("self")), rVal);
    } else if (auto ai = lValue->asArrayIndexExpression()) {
      bldr << location;
      ai->generateStore(file, bldr, rVal);
    } else if (auto ma = lValue->asMemberAccessExpression()) {
      bldr << location;
      ma->generateStore(file, bldr, rVal);
    } else {
      CapricaReportingContext::logicalFatal("Invalid Lefthand Side for PapyrusAssignStatement!");
    }
    if (capturedIndex)
      bldr.freeLongLivedTemp(capturedIndex->local);
    if (capturedBase)
      bldr.freeLongLivedTemp(capturedBase->local);
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    if (auto id = lValue->asIdentifierExpression())
      id->isAssignmentContext = true;

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
      if (lValue->resultType().type == PapyrusType::Kind::Array && !conf::Papyrus::enableLanguageExtensions) {
        ctx->reportingContext.error(
            location,
            "You can't do anything except assign to an array element unless you have language extensions enabled!");
      }
      rValue = ctx->coerceExpression(binOpExpression, lValue->resultType());

      // The read and write must use the same receiver and index, even if the RHS changes them.
      if (auto ai = lValue->asArrayIndexExpression()) {
        capturedBase = ctx->allocator->make<CapturedExpression>(ai->baseExpression);
        ai->baseExpression = capturedBase;
        if (!ai->indexExpression->asLiteralExpression()) {
          capturedIndex = ctx->allocator->make<CapturedExpression>(ai->indexExpression);
          ai->indexExpression = capturedIndex;
        }
      } else if (auto ma = lValue->asMemberAccessExpression()) {
        if (!ma->baseExpression->asSelfExpression() && !ma->baseExpression->asParentExpression()) {
          capturedBase = ctx->allocator->make<CapturedExpression>(ma->baseExpression);
          ma->baseExpression = capturedBase;
        }
      }
    }

    if (auto id = lValue->asIdentifierExpression()) {
      id->identifier.ensureAssignable(ctx->reportingContext);
      id->identifier.markWritten();
    } else if (auto ai = lValue->asArrayIndexExpression()) {
      // It's valid.
    } else if (auto ma = lValue->asMemberAccessExpression()) {
      if (auto ident = ma->accessExpression->asIdentifierExpression())
        ident->identifier.ensureAssignable(ctx->reportingContext);
    } else {
      ctx->reportingContext.error(lValue->location, "Invalid Lefthand Side for PapyrusAssignStatement!");
    }
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override { visitor.visit(this); }
};

}}}
