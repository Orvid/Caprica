#pragma once

#include <papyrus/expressions/PapyrusExpression.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

enum class PapyrusBinaryOperatorType {
  None,

  BooleanOr,
  BooleanAnd,

  CmpEq,
  CmpNeq,
  CmpLt,
  CmpLte,
  CmpGt,
  CmpGte,

  Add,
  Subtract,

  Multiply,
  Divide,
  Modulus,
};

struct PapyrusBinaryOpExpression final : public PapyrusExpression {
  PapyrusExpression* left { nullptr };
  PapyrusBinaryOperatorType operation { PapyrusBinaryOperatorType::None };
  PapyrusExpression* right { nullptr };

  explicit PapyrusBinaryOpExpression(const CapricaFileLocation& loc) : PapyrusExpression(loc) { }
  PapyrusBinaryOpExpression(const PapyrusBinaryOpExpression&) = delete;
  virtual ~PapyrusBinaryOpExpression() override = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    auto lVal = left->generateLoad(file, bldr);
    auto dest = bldr.allocTemp(this->resultType());
    if (operation == PapyrusBinaryOperatorType::BooleanOr) {
      bldr << location;
      bldr << op::assign { dest, lVal };
      pex::PexLabel* after;
      bldr >> after;
      bldr << op::jmpt { dest, after };
      auto rVal = right->generateLoad(file, bldr);
      bldr << location;
      bldr << op::assign { dest, rVal };
      bldr << after;
    } else if (operation == PapyrusBinaryOperatorType::BooleanAnd) {
      bldr << location;
      bldr << op::assign { dest, lVal };
      pex::PexLabel* afterAll;
      bldr >> afterAll;
      bldr << op::jmpf { dest, afterAll };
      auto rVal = right->generateLoad(file, bldr);
      bldr << location;
      bldr << op::assign { dest, rVal };
      bldr << afterAll;
    } else {
      auto rVal = right->generateLoad(file, bldr);
      bldr << location;
      switch (operation) {
        case PapyrusBinaryOperatorType::CmpEq:
          bldr << op::cmpeq { dest, lVal, rVal };
          return dest;
        case PapyrusBinaryOperatorType::CmpNeq:
          bldr << op::cmpeq { dest, lVal, rVal };
          bldr << op::_not { dest, dest };
          return dest;
        case PapyrusBinaryOperatorType::CmpLt:
          bldr << op::cmplt { dest, lVal, rVal };
          return dest;
        case PapyrusBinaryOperatorType::CmpLte:
          bldr << op::cmplte { dest, lVal, rVal };
          return dest;
        case PapyrusBinaryOperatorType::CmpGt:
          bldr << op::cmpgt { dest, lVal, rVal };
          return dest;
        case PapyrusBinaryOperatorType::CmpGte:
          bldr << op::cmpgte { dest, lVal, rVal };
          return dest;

        case PapyrusBinaryOperatorType::Add:
          if (left->resultType().type == PapyrusType::Kind::Int)
            bldr << op::iadd { dest, lVal, rVal };
          else if (left->resultType().type == PapyrusType::Kind::Float)
            bldr << op::fadd { dest, lVal, rVal };
          else if (left->resultType().type == PapyrusType::Kind::String)
            bldr << op::strcat { dest, lVal, rVal };
          else
            bldr.reportingContext.fatal(location, "Unknown argument type to an add operation!");
          return dest;

        case PapyrusBinaryOperatorType::Subtract:
          if (left->resultType().type == PapyrusType::Kind::Int)
            bldr << op::isub { dest, lVal, rVal };
          else if (left->resultType().type == PapyrusType::Kind::Float)
            bldr << op::fsub { dest, lVal, rVal };
          else
            bldr.reportingContext.fatal(location, "Unknown argument type to a subtraction operation!");
          return dest;

        case PapyrusBinaryOperatorType::Multiply:
          if (left->resultType().type == PapyrusType::Kind::Int)
            bldr << op::imul { dest, lVal, rVal };
          else if (left->resultType().type == PapyrusType::Kind::Float)
            bldr << op::fmul { dest, lVal, rVal };
          else
            bldr.reportingContext.fatal(location, "Unknown argument type to a multiplication operation!");
          return dest;

        case PapyrusBinaryOperatorType::Divide:
          if (left->resultType().type == PapyrusType::Kind::Int)
            bldr << op::idiv { dest, lVal, rVal };
          else if (left->resultType().type == PapyrusType::Kind::Float)
            bldr << op::fdiv { dest, lVal, rVal };
          else
            bldr.reportingContext.fatal(location, "Unknown argument type to a division operation!");
          return dest;

        case PapyrusBinaryOperatorType::Modulus:
          if (left->resultType().type != PapyrusType::Kind::Int)
            bldr.reportingContext.fatal(location, "Unknown argument type to a modulus operation!");
          bldr << op::imod { dest, lVal, rVal };
          return dest;

        case PapyrusBinaryOperatorType::BooleanOr:
        case PapyrusBinaryOperatorType::BooleanAnd:
        case PapyrusBinaryOperatorType::None:
          break;
      }
      CapricaReportingContext::logicalFatal("Unknown PapyrusBinaryOperatorType while generating the pex opcodes!");
    }
    return dest;
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    assert(operation != PapyrusBinaryOperatorType::None);
    left->semantic(ctx);
    ctx->checkForPoison(left);
    right->semantic(ctx);
    ctx->checkForPoison(right);
    switch (operation) {
      case PapyrusBinaryOperatorType::BooleanOr:
      case PapyrusBinaryOperatorType::BooleanAnd:
        left = ctx->coerceExpression(left, PapyrusType::Bool(left->location));
        right = ctx->coerceExpression(right, PapyrusType::Bool(right->location));
        return;

      case PapyrusBinaryOperatorType::CmpEq:
      case PapyrusBinaryOperatorType::CmpNeq:
      case PapyrusBinaryOperatorType::Add:
        coerceToSameType(ctx);
        return;

      case PapyrusBinaryOperatorType::CmpLt:
      case PapyrusBinaryOperatorType::CmpLte:
      case PapyrusBinaryOperatorType::CmpGt:
      case PapyrusBinaryOperatorType::CmpGte:
      case PapyrusBinaryOperatorType::Subtract:
      case PapyrusBinaryOperatorType::Multiply:
      case PapyrusBinaryOperatorType::Divide:
        coerceToSameType(ctx);
        if (left->resultType().type != PapyrusType::Kind::Int && left->resultType().type != PapyrusType::Kind::Float) {
          ctx->reportingContext.fatal(
              location,
              "The <, <=, >, >=, -, *, /, and % operators are only valid on integers and floats!");
        }
        return;

      case PapyrusBinaryOperatorType::Modulus:
        coerceToSameType(ctx);
        if (left->resultType().type != PapyrusType::Kind::Int)
          ctx->reportingContext.fatal(location, "The modulus operator can only be used on integers!");
        return;

      case PapyrusBinaryOperatorType::None:
        break;
    }
    CapricaReportingContext::logicalFatal("Unknown PapyrusBinaryOperatorType in semantic pass!");
  }

  virtual PapyrusType resultType() const override {
    // This is dependent on the operator.
    switch (operation) {
      case PapyrusBinaryOperatorType::BooleanOr:
      case PapyrusBinaryOperatorType::BooleanAnd:
      case PapyrusBinaryOperatorType::CmpEq:
      case PapyrusBinaryOperatorType::CmpNeq:
      case PapyrusBinaryOperatorType::CmpLt:
      case PapyrusBinaryOperatorType::CmpLte:
      case PapyrusBinaryOperatorType::CmpGt:
      case PapyrusBinaryOperatorType::CmpGte:
        return PapyrusType::Bool(location);

      case PapyrusBinaryOperatorType::Add:
      case PapyrusBinaryOperatorType::Subtract:
      case PapyrusBinaryOperatorType::Multiply:
      case PapyrusBinaryOperatorType::Divide:
      case PapyrusBinaryOperatorType::Modulus:
        return left->resultType();

      case PapyrusBinaryOperatorType::None:
        break;
    }
    CapricaReportingContext::logicalFatal("Unknown PapyrusBinaryOperatorType!");
  }

private:
  void coerceToSameType(PapyrusResolutionContext* ctx) {
    if (left->resultType().type == PapyrusType::Kind::String || right->resultType().type == PapyrusType::Kind::String) {
      left = ctx->coerceExpression(left, PapyrusType::String(left->location));
      right = ctx->coerceExpression(right, PapyrusType::String(right->location));
    } else if (left->resultType().type == PapyrusType::Kind::Bool ||
               right->resultType().type == PapyrusType::Kind::Bool) {
      left = ctx->coerceExpression(left, PapyrusType::Bool(left->location));
      right = ctx->coerceExpression(right, PapyrusType::Bool(right->location));
    } else if (left->resultType().type == PapyrusType::Kind::Float ||
               right->resultType().type == PapyrusType::Kind::Float) {
      left = ctx->coerceExpression(left, PapyrusType::Float(left->location));
      right = ctx->coerceExpression(right, PapyrusType::Float(right->location));
    } else {
      if (!ctx->canImplicitlyCoerceExpression(right, left->resultType()))
        left = ctx->coerceExpression(left, right->resultType());
      else
        right = ctx->coerceExpression(right, left->resultType());
    }
  }
};

}}}
