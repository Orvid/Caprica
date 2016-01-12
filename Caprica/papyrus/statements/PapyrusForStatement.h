#pragma once

#include <vector>

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
  PapyrusValue initialValue;
  PapyrusValue targetValue;
  std::vector<PapyrusStatement*> body{ };

  PapyrusForStatement(const CapricaFileLocation& loc, const PapyrusValue& initVal, const PapyrusValue& targVal) : PapyrusStatement(loc), initialValue(initVal), targetValue(targVal) { }
  virtual ~PapyrusForStatement() override {
    if (declareStatement)
      delete declareStatement;
    if (iteratorVariable)
      delete iteratorVariable;
    for (auto s : body)
      delete s;
  }

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    namespace op = caprica::pex::op;
    pex::PexLabel* beforeCondition;
    bldr >> beforeCondition;
    pex::PexLabel* afterAll;
    bldr >> afterAll;
    bldr.pushBreakContinueScope(afterAll, beforeCondition);

    pex::PexValue loadedCounter;
    if (declareStatement) {
      auto loc = bldr.allocateLocal(declareStatement->name, declareStatement->type);
      bldr << location;
      bldr << op::assign{ loc, initialValue.buildPex(file) };
      loadedCounter = loc;
    } else {
      iteratorVariable->generateStore(file, bldr, pex::PexValue::Identifier(file->getString("self")), initialValue.buildPex(file));
    }
    bldr << location;
    bldr << beforeCondition;
    if (iteratorVariable)
      loadedCounter = iteratorVariable->generateLoad(file, bldr, pex::PexValue::Identifier(file->getString("self")));
    auto bTemp = bldr.allocTemp(PapyrusType::Bool(location));
    if (initialValue.i < targetValue.i)
      bldr << op::cmplte{ bTemp, loadedCounter, targetValue.buildPex(file) };
    else
      bldr << op::cmpgte{ bTemp, loadedCounter, targetValue.buildPex(file) };
    bldr << op::jmpf{ bTemp, afterAll };

    for (auto s : body)
      s->buildPex(file, bldr);

    bldr << location;
    if (initialValue.i < targetValue.i) {
      if (declareStatement) {
        bldr << op::iadd{ pex::PexValue::Identifier::fromVar(loadedCounter), loadedCounter, pex::PexValue::Integer(1) };
      } else {
        loadedCounter = iteratorVariable->generateLoad(file, bldr, pex::PexValue::Identifier(file->getString("self")));
        auto tmp = bldr.allocTemp(iteratorVariable->resultType());
        bldr << op::iadd{ tmp, loadedCounter, pex::PexValue::Integer(1) };
        iteratorVariable->generateStore(file, bldr, pex::PexValue::Identifier(file->getString("self")), tmp);
      }
    } else {
      if (declareStatement) {
        bldr << op::isub{ pex::PexValue::Identifier::fromVar(loadedCounter), loadedCounter, pex::PexValue::Integer(1) };
      } else {
        loadedCounter = iteratorVariable->generateLoad(file, bldr, pex::PexValue::Identifier(file->getString("self")));
        auto tmp = bldr.allocTemp(iteratorVariable->resultType());
        bldr << op::isub{ tmp, loadedCounter, pex::PexValue::Integer(1) };
        iteratorVariable->generateStore(file, bldr, pex::PexValue::Identifier(file->getString("self")), tmp);
      }
    }
    bldr << op::jmp{ beforeCondition };
    bldr.popBreakContinueScope();
    bldr << afterAll;
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    if (initialValue.getPapyrusType().type != PapyrusType::Kind::Int)
      CapricaError::error(initialValue.location, "For statements only support Int counter values, got an initial value of type '%s'!", initialValue.getPapyrusType().prettyString().c_str());
    if (targetValue.getPapyrusType().type != PapyrusType::Kind::Int)
      CapricaError::error(initialValue.location, "For statements only support Int counter values, got a target value of type '%s'!", targetValue.getPapyrusType().prettyString().c_str());

    ctx->pushBreakContinueScope();
    ctx->pushLocalVariableScope();
    if (declareStatement) {
      if (declareStatement->isAuto) {
        declareStatement->type = PapyrusType::Int(declareStatement->location);
        declareStatement->isAuto = false;
      }
      declareStatement->semantic(ctx);
      if (declareStatement->type.type != PapyrusType::Kind::Int)
        CapricaError::error(initialValue.location, "For statements only support Int counter values, got a counter of type '%s'!", declareStatement->type.prettyString().c_str());
    } else {
      *iteratorVariable = ctx->resolveIdentifier(*iteratorVariable);
      if (iteratorVariable->resultType().type != PapyrusType::Kind::Int)
        CapricaError::error(initialValue.location, "For statements only support Int counter values, got a counter of type '%s'!", iteratorVariable->resultType().prettyString().c_str());
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
