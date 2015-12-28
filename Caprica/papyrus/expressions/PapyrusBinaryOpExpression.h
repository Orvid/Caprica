#pragma once

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/parser/PapyrusFileLocation.h>

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

  PapyrusBinaryOpExpression(parser::PapyrusFileLocation loc) : PapyrusExpression(loc) { }
  ~PapyrusBinaryOpExpression() {
    if (left)
      delete left;
    if (right)
      delete right;
  }

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    auto lVal = left->generateLoad(file, bldr);
    auto rVal = right->generateLoad(file, bldr);
    auto dest = bldr.allocTemp(file, this->resultType());
    bldr << location;
    switch (operation) {
      case PapyrusBinaryOperatorType::BooleanOr:
      case PapyrusBinaryOperatorType::BooleanAnd:
        //left = coerceExpression(left, PapyrusType::Bool());
        //right = coerceExpression(right, PapyrusType::Bool());
        break;

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
        if (left->resultType() == PapyrusType::Int())
          bldr << op::iadd{ dest, lVal, rVal };
        else if (left->resultType() == PapyrusType::Float())
          bldr << op::fadd{ dest, lVal, rVal };
        else if (left->resultType() == PapyrusType::String())
          bldr << op::strcat{ dest, lVal, rVal };
        else
          throw std::runtime_error("Unknown argument type to an add operation!");
        break;
      case PapyrusBinaryOperatorType::Subtract:
        if (left->resultType() == PapyrusType::Int())
          bldr << op::isub{ dest, lVal, rVal };
        else if (left->resultType() == PapyrusType::Float())
          bldr << op::fsub{ dest, lVal, rVal };
        else
          throw std::runtime_error("Unknown argument type to an add operation!");
        break;
      case PapyrusBinaryOperatorType::Multiply:
        if (left->resultType() == PapyrusType::Int())
          bldr << op::imul{ dest, lVal, rVal };
        else if (left->resultType() == PapyrusType::Float())
          bldr << op::fmul{ dest, lVal, rVal };
        else
          throw std::runtime_error("Unknown argument type to an add operation!");
        break;
      case PapyrusBinaryOperatorType::Divide:
        if (left->resultType() == PapyrusType::Int())
          bldr << op::idiv{ dest, lVal, rVal };
        else if (left->resultType() == PapyrusType::Float())
          bldr << op::fdiv{ dest, lVal, rVal };
        else
          throw std::runtime_error("Unknown argument type to an add operation!");
        break;
      case PapyrusBinaryOperatorType::Modulus:
        if (left->resultType() != PapyrusType::Int())
          throw std::runtime_error("Unknown argument type to an add operation!");
        bldr << op::imod{ dest, lVal, rVal };
        break;

      default:
        throw std::runtime_error("Unknown PapyrusBinaryOperatorType while generating the pex opcodes!");
    }
    bldr.freeIfTemp(lVal);
    bldr.freeIfTemp(rVal);
    return dest;
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    assert(operation != PapyrusBinaryOperatorType::None);
    left->semantic(ctx);
    right->semantic(ctx);
    switch (operation) {
      case PapyrusBinaryOperatorType::BooleanOr:
      case PapyrusBinaryOperatorType::BooleanAnd:
        left = coerceExpression(left, PapyrusType::Bool());
        right = coerceExpression(right, PapyrusType::Bool());
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
        if (left->resultType() != PapyrusType::Int() && left->resultType() != PapyrusType::Float())
          ctx->fatalError("The <, <=, >, >=, -, *, /, and % operators are only valid on integers and floats!");
        break;

      case PapyrusBinaryOperatorType::Modulus:
        coerceToSameType();
        if (left->resultType() != PapyrusType::Int())
          ctx->fatalError("The modulus operator can only be used on integers!");
        break;

      default:
        throw std::runtime_error("Unknown PapyrusBinaryOperatorType in semantic pass!");
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
        return PapyrusType::Bool();

      case PapyrusBinaryOperatorType::Add:
      case PapyrusBinaryOperatorType::Subtract:
      case PapyrusBinaryOperatorType::Multiply:
      case PapyrusBinaryOperatorType::Divide:
      case PapyrusBinaryOperatorType::Modulus:
        return left->resultType();

      default:
        throw std::runtime_error("Unknown PapyrusBinaryOperatorType!");
    }
  }

private:
  void coerceToSameType() {
    if (left->resultType() == PapyrusType::String() || right->resultType() == PapyrusType::String()) {
      left = coerceExpression(left, PapyrusType::String());
      right = coerceExpression(right, PapyrusType::String());
    } else if (left->resultType() == PapyrusType::Bool() || right->resultType() == PapyrusType::Bool()) {
      left = coerceExpression(left, PapyrusType::Bool());
      right = coerceExpression(right, PapyrusType::Bool());
    } else if (left->resultType() == PapyrusType::Float() || right->resultType() == PapyrusType::Float()) {
      left = coerceExpression(left, PapyrusType::Float());
      right = coerceExpression(right, PapyrusType::Float());
    } else {
      right = coerceExpression(right, left->resultType());
    }
  }
};

}}}
