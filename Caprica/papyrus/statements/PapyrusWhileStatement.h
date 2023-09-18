#pragma once

#include <common/CapricaFileLocation.h>
#include <common/IntrusiveLinkedList.h>

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusWhileStatement final : public PapyrusStatement
{
  expressions::PapyrusExpression* condition{ nullptr };
  IntrusiveLinkedList<PapyrusStatement> body{ };

  explicit PapyrusWhileStatement(CapricaFileLocation loc) : PapyrusStatement(loc) { }
  PapyrusWhileStatement(const PapyrusWhileStatement&) = delete;
  virtual ~PapyrusWhileStatement() override = default;

  virtual bool buildCFG(PapyrusCFG& cfg) const override {
    return cfg.processCommonLoopBody(body);
  }

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    pex::PexLabel* beforeCondition;
    bldr >> beforeCondition;
    bldr << beforeCondition;
    pex::PexLabel* afterAll;
    bldr >> afterAll;
    bldr.pushBreakContinueScope(afterAll, beforeCondition);
    auto lVal = condition->generateLoad(file, bldr);
    bldr << location;
    bldr << op::jmpf{ lVal, afterAll };

    for (auto s : body)
      s->buildPex(file, bldr);
    bldr << location;
    bldr << op::jmp{ beforeCondition };

    bldr << afterAll;
    bldr.popBreakContinueScope();
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    condition->semantic(ctx);
    ctx->checkForPoison(condition);
    condition = ctx->coerceExpression(condition, PapyrusType::Bool(condition->location));
    ctx->pushBreakContinueScope();
    ctx->pushLocalVariableScope();
    if (conf::Papyrus::game == GameID::Skyrim) {
      for (auto s: body) {
        s->semantic_skyrim_first_pass(ctx);
      }
    }
    for (auto s : body)
      s->semantic(ctx);
    ctx->popLocalVariableScope();
    ctx->popBreakContinueScope();
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);

    for (auto s : body)
      s->visit(visitor);
  }
};

}}}
