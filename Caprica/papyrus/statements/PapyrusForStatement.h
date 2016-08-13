#pragma once

#include <common/IntrusiveLinkedList.h>

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusDeclareStatement.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusForStatement final : public PapyrusStatement
{
  PapyrusDeclareStatement* declareStatement{ nullptr };
  PapyrusIdentifier* iteratorVariable{ nullptr };
  expressions::PapyrusExpression* initialValue{ nullptr };
  expressions::PapyrusExpression* targetValue{ nullptr };
  expressions::PapyrusExpression* stepValue{ nullptr };
  IntrusiveLinkedList<PapyrusStatement> body{ };

  explicit PapyrusForStatement(const CapricaFileLocation& loc) : PapyrusStatement(loc) { }
  PapyrusForStatement(const PapyrusForStatement&) = delete;
  virtual ~PapyrusForStatement() override = default;

  virtual bool buildCFG(PapyrusCFG& cfg) const override {
    if (declareStatement)
      cfg.appendStatement(declareStatement);
    return cfg.processCommonLoopBody(body);
  }

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    namespace op = caprica::pex::op;
    pex::PexLabel* beforeCondition;
    bldr >> beforeCondition;
    pex::PexLabel* afterAll;
    bldr >> afterAll;
    pex::PexLabel* continueLabel;
    bldr >> continueLabel;
    bldr.pushBreakContinueScope(afterAll, continueLabel);

    pex::PexValue loadedCounter;
    auto iVal = initialValue->generateLoad(file, bldr);
    if (declareStatement) {
      auto loc = bldr.allocateLocal(declareStatement->name, declareStatement->type);
      bldr << location;
      bldr << op::assign{ loc, iVal };
      loadedCounter = loc;
    } else {
      iteratorVariable->generateStore(file, bldr, pex::PexValue::Identifier(file->getString("self")), iVal);
    }
    
    pex::PexLocalVariable* sValLoc{ nullptr };
    pex::PexValue sVal;
    if (stepValue && !stepValue->asLiteralExpression()) {
      auto stepValVal = stepValue->generateLoad(file, bldr);
      bldr << location;
      sValLoc = bldr.allocLongLivedTemp(initialValue->resultType());
      bldr << op::assign{ sValLoc, stepValVal };
      sVal = sValLoc;
    } else {
      bldr << location;
      if (stepValue) // It's a literal expression, inline it.
        sVal = stepValue->generateLoad(file, bldr);
      else if (initialValue->resultType().type == PapyrusType::Kind::Int)
        sVal = pex::PexValue::Integer(1);
      else
        sVal = pex::PexValue::Float(1);
    }

    pex::PexLocalVariable* tValLoc{ nullptr };
    pex::PexValue tVal;
    if (!targetValue->asLiteralExpression()) {
      tValLoc = bldr.allocLongLivedTemp(targetValue->resultType());
      auto targValVal = targetValue->generateLoad(file, bldr);
      bldr << location;
      bldr << op::assign{ tValLoc, targValVal };
      tVal = tValLoc;
    } else {
      tVal = targetValue->generateLoad(file, bldr);
    }

    pex::PexLocalVariable* lowerStep{ nullptr };
    if (sValLoc) {
      lowerStep = bldr.allocLongLivedTemp(PapyrusType::Bool(location));
      if (initialValue->resultType().type == PapyrusType::Kind::Int)
        bldr << op::cmpgt{ lowerStep, sVal, pex::PexValue::Integer(0) };
      else
        bldr << op::cmpgt{ lowerStep, sVal, pex::PexValue::Float(0) };
    }

    bldr << beforeCondition;
    if (iteratorVariable)
      loadedCounter = iteratorVariable->generateLoad(file, bldr, pex::PexValue::Identifier(file->getString("self")));
    auto bTemp = bldr.allocTemp(PapyrusType::Bool(location));
    if (sValLoc) {
      auto aComp = bldr.label();
      auto gtComp = bldr.label();
      bldr << op::jmpf{ lowerStep, gtComp };
      bldr << op::cmplte{ bTemp, loadedCounter, tVal };
      bldr << op::jmp{ aComp };
      bldr << gtComp;
      bldr << op::cmpgte{ bTemp, loadedCounter, tVal };
      bldr << aComp;
    } else {
      if ((sVal.type == pex::PexValueType::Integer && sVal.val.i > 0) || (sVal.type == pex::PexValueType::Float && sVal.val.f > 0))
        bldr << op::cmplte{ bTemp, loadedCounter, tVal };
      else if ((sVal.type == pex::PexValueType::Integer && sVal.val.i < 0) || (sVal.type == pex::PexValueType::Float && sVal.val.f < 0))
        bldr << op::cmpgte{ bTemp, loadedCounter, tVal };
      else
        bldr.reportingContext.fatal(location, "Attempted to step by a literal 0!");
    }
    bldr << op::jmpf{ bTemp, afterAll };

    for (auto s : body)
      s->buildPex(file, bldr);

    bldr << continueLabel;
    bldr << location;
    if (declareStatement) {
      if (declareStatement->type.type == PapyrusType::Kind::Int)
        bldr << op::iadd{ pex::PexValue::Identifier::fromVar(loadedCounter), loadedCounter, sVal };
      else
        bldr << op::fadd{ pex::PexValue::Identifier::fromVar(loadedCounter), loadedCounter, sVal };
    } else {
      loadedCounter = iteratorVariable->generateLoad(file, bldr, pex::PexValue::Identifier(file->getString("self")));
      auto tmp = bldr.allocTemp(iteratorVariable->resultType());
      if (iteratorVariable->resultType().type == PapyrusType::Kind::Int)
        bldr << op::iadd{ tmp, loadedCounter, sVal };
      else
        bldr << op::fadd{ tmp, loadedCounter, sVal };
      iteratorVariable->generateStore(file, bldr, pex::PexValue::Identifier(file->getString("self")), tmp);
      bldr << location;
    }
    bldr << op::jmp{ beforeCondition };
    if (sValLoc)
      bldr.freeLongLivedTemp(sValLoc);
    if (lowerStep)
      bldr.freeLongLivedTemp(lowerStep);
    if (tValLoc)
      bldr.freeLongLivedTemp(tValLoc);
    bldr.popBreakContinueScope();
    bldr << afterAll;
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    initialValue->semantic(ctx);
    ctx->checkForPoison(initialValue);
    if (initialValue->resultType().type != PapyrusType::Kind::Int && initialValue->resultType().type != PapyrusType::Kind::Float)
      ctx->reportingContext.error(initialValue->location, "For statements only support Int and Float counter values, got an initial value of type '%s'!", initialValue->resultType().prettyString().c_str());
    targetValue->semantic(ctx);
    ctx->checkForPoison(targetValue);
    if (targetValue->resultType().type != PapyrusType::Kind::Int && targetValue->resultType().type != PapyrusType::Kind::Float)
      ctx->reportingContext.error(initialValue->location, "For statements only support Int and Float counter values, got a target value of type '%s'!", targetValue->resultType().prettyString().c_str());
    // TODO: Allow some implicit coercion and add checks about the declare/iterator/step expressions' types as well.
    if (targetValue->resultType() != initialValue->resultType())
      ctx->reportingContext.error(initialValue->location, "The intial value, of type '%s', does not match the target value type '%s'!", initialValue->resultType().prettyString().c_str(), targetValue->resultType().prettyString().c_str());
    if (stepValue) {
      stepValue->semantic(ctx);
      ctx->checkForPoison(stepValue);
      if (stepValue->resultType().type != PapyrusType::Kind::Int && stepValue->resultType().type != PapyrusType::Kind::Float)
        ctx->reportingContext.error(initialValue->location, "For statements only support Int and Float counter values, got a step value of type '%s'!", stepValue->resultType().prettyString().c_str());
    }

    ctx->pushBreakContinueScope();
    ctx->pushLocalVariableScope();
    if (declareStatement) {
      if (declareStatement->isAuto) {
        declareStatement->type = initialValue->resultType();
        declareStatement->isAuto = false;
      }
      declareStatement->semantic(ctx);
      if (declareStatement->type.type != PapyrusType::Kind::Int && declareStatement->type.type != PapyrusType::Kind::Float)
        ctx->reportingContext.error(initialValue->location, "For statements only support Int and Float counter values, got a counter of type '%s'!", declareStatement->type.prettyString().c_str());
    } else {
      *iteratorVariable = ctx->resolveIdentifier(*iteratorVariable);
      iteratorVariable->ensureAssignable(ctx->reportingContext);
      iteratorVariable->markWritten();
      if (iteratorVariable->resultType().type != PapyrusType::Kind::Int && iteratorVariable->resultType().type != PapyrusType::Kind::Float)
        ctx->reportingContext.error(initialValue->location, "For statements only support Int and Float counter values, got a counter of type '%s'!", iteratorVariable->resultType().prettyString().c_str());
    }

    for (auto s : body)
      s->semantic(ctx);
    ctx->popLocalVariableScope();
    ctx->popBreakContinueScope();
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);

    if (declareStatement)
      declareStatement->visit(visitor);
    
    for (auto s : body)
      s->visit(visitor);
  }
};

}}}
