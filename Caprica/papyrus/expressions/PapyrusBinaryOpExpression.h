#pragma once

#include <papyrus/expressions/PapyrusExpression.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

enum class PapyrusBinaryOperatorType
{
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

struct PapyrusBinaryOpExpression final : public PapyrusExpression
{
  PapyrusExpression* left{ nullptr };
  PapyrusBinaryOperatorType operation{ PapyrusBinaryOperatorType::None };
  PapyrusExpression* right{ nullptr };

  explicit PapyrusBinaryOpExpression(const CapricaFileLocation& loc) : PapyrusExpression(loc) { }
  PapyrusBinaryOpExpression(const PapyrusBinaryOpExpression&) = delete;
  virtual ~PapyrusBinaryOpExpression() override {
    if (left)
      delete left;
    if (right)
      delete right;
  }

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    auto lVal = left->generateLoad(file, bldr);
    auto dest = bldr.allocTemp(this->resultType());
    if (operation == PapyrusBinaryOperatorType::BooleanOr) {
      bldr << location;
      bldr << op::assign{ dest, lVal };
      pex::PexLabel* after;
      bldr >> after;
      bldr << op::jmpt{ dest, after };
      auto rVal = right->generateLoad(file, bldr);
      bldr << location;
      bldr << op::assign{ dest, rVal };
      bldr << after;
    } else if (operation == PapyrusBinaryOperatorType::BooleanAnd) {
      bldr << location;
      bldr << op::assign{ dest, lVal };
      pex::PexLabel* afterAll;
      bldr >> afterAll;
      bldr << op::jmpf{ dest, afterAll };
      auto rVal = right->generateLoad(file, bldr);
      bldr << location;
      bldr << op::assign{ dest, rVal };
      bldr << afterAll;
    } else {
      auto rVal = right->generateLoad(file, bldr);
      bldr << location;
      switch (operation) {
        case PapyrusBinaryOperatorType::CmpEq:
          bldr << op::cmpeq{ dest, lVal, rVal };
          break;
        case PapyrusBinaryOperatorType::CmpNeq:
          bldr << op::cmpeq{ dest, lVal, rVal };
          bldr << op::not{ dest, dest };
          break;
        case PapyrusBinaryOperatorType::CmpLt:
          bldr << op::cmplt{ dest, lVal, rVal };
          break;
        case PapyrusBinaryOperatorType::CmpLte:
          bldr << op::cmplte{ dest, lVal, rVal };
          break;
        case PapyrusBinaryOperatorType::CmpGt:
          bldr << op::cmpgt{ dest, lVal, rVal };
          break;
        case PapyrusBinaryOperatorType::CmpGte:
          bldr << op::cmpgte{ dest, lVal, rVal };
          break;

        case PapyrusBinaryOperatorType::Add:
          if (left->resultType().type == PapyrusType::Kind::Int)
            bldr << op::iadd{ dest, lVal, rVal };
          else if (left->resultType().type == PapyrusType::Kind::Float)
            bldr << op::fadd{ dest, lVal, rVal };
          else if (left->resultType().type == PapyrusType::Kind::String)
            bldr << op::strcat{ dest, lVal, rVal };
          else
            CapricaError::fatal(location, "Unknown argument type to an add operation!");
          break;

        case PapyrusBinaryOperatorType::Subtract:
          if (left->resultType().type == PapyrusType::Kind::Int)
            bldr << op::isub{ dest, lVal, rVal };
          else if (left->resultType().type == PapyrusType::Kind::Float)
            bldr << op::fsub{ dest, lVal, rVal };
          else
            CapricaError::fatal(location, "Unknown argument type to a subtraction operation!");
          break;

        case PapyrusBinaryOperatorType::Multiply:
          if (left->resultType().type == PapyrusType::Kind::Int)
            bldr << op::imul{ dest, lVal, rVal };
          else if (left->resultType().type == PapyrusType::Kind::Float)
            bldr << op::fmul{ dest, lVal, rVal };
          else
            CapricaError::fatal(location, "Unknown argument type to a multiplication operation!");
          break;

        case PapyrusBinaryOperatorType::Divide:
          if (left->resultType().type == PapyrusType::Kind::Int)
            bldr << op::idiv{ dest, lVal, rVal };
          else if (left->resultType().type == PapyrusType::Kind::Float)
            bldr << op::fdiv{ dest, lVal, rVal };
          else
            CapricaError::fatal(location, "Unknown argument type to a division operation!");
          break;

        case PapyrusBinaryOperatorType::Modulus:
          if (left->resultType().type != PapyrusType::Kind::Int)
            CapricaError::fatal(location, "Unknown argument type to a modulus operation!");
          bldr << op::imod{ dest, lVal, rVal };
          break;

        default:
          CapricaError::logicalFatal("Unknown PapyrusBinaryOperatorType while generating the pex opcodes!");
      }
    }
    return dest;
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    assert(operation != PapyrusBinaryOperatorType::None);
    left->semantic(ctx);
    right->semantic(ctx);
    switch (operation) {
      case PapyrusBinaryOperatorType::BooleanOr:
      case PapyrusBinaryOperatorType::BooleanAnd:
        left = PapyrusResolutionContext::coerceExpression(left, PapyrusType::Bool(left->location));
        right = PapyrusResolutionContext::coerceExpression(right, PapyrusType::Bool(right->location));
        break;

      case PapyrusBinaryOperatorType::CmpEq:
      case PapyrusBinaryOperatorType::CmpNeq:
      case PapyrusBinaryOperatorType::Add:
        coerceToSameType();
        break;

      case PapyrusBinaryOperatorType::CmpLt:
      case PapyrusBinaryOperatorType::CmpLte:
      case PapyrusBinaryOperatorType::CmpGt:
      case PapyrusBinaryOperatorType::CmpGte:
      case PapyrusBinaryOperatorType::Subtract:
      case PapyrusBinaryOperatorType::Multiply:
      case PapyrusBinaryOperatorType::Divide:
        coerceToSameType();
        if (left->resultType().type != PapyrusType::Kind::Int && left->resultType().type != PapyrusType::Kind::Float)
          CapricaError::fatal(location, "The <, <=, >, >=, -, *, /, and % operators are only valid on integers and floats!");
        break;

      case PapyrusBinaryOperatorType::Modulus:
        coerceToSameType();
        if (left->resultType().type != PapyrusType::Kind::Int)
          CapricaError::fatal(location, "The modulus operator can only be used on integers!");
        break;

      default:
        CapricaError::logicalFatal("Unknown PapyrusBinaryOperatorType in semantic pass!");
    }
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

      default:
        CapricaError::logicalFatal("Unknown PapyrusBinaryOperatorType!");
    }
  }

private:
  void coerceToSameType() {
    if (left->resultType().type == PapyrusType::Kind::String || right->resultType().type == PapyrusType::Kind::String) {
      left = PapyrusResolutionContext::coerceExpression(left, PapyrusType::String(left->location));
      right = PapyrusResolutionContext::coerceExpression(right, PapyrusType::String(right->location));
    } else if (left->resultType().type == PapyrusType::Kind::Bool || right->resultType().type == PapyrusType::Kind::Bool) {
      left = PapyrusResolutionContext::coerceExpression(left, PapyrusType::Bool(left->location));
      right = PapyrusResolutionContext::coerceExpression(right, PapyrusType::Bool(right->location));
    } else if (left->resultType().type == PapyrusType::Kind::Float || right->resultType().type == PapyrusType::Kind::Float) {
      left = PapyrusResolutionContext::coerceExpression(left, PapyrusType::Float(left->location));
      right = PapyrusResolutionContext::coerceExpression(right, PapyrusType::Float(right->location));
    } else {
      right = PapyrusResolutionContext::coerceExpression(right, left->resultType());
    }
  }
};

}}}
