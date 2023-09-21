#pragma once

#include <common/IntrusiveLinkedList.h>

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusIfStatement final : public PapyrusStatement
{
  struct IfBody final
  {
    expressions::PapyrusExpression* condition{ nullptr };
    IntrusiveLinkedList<PapyrusStatement> body{ };

    IfBody(expressions::PapyrusExpression* c, IntrusiveLinkedList<PapyrusStatement>&& b) : condition(c), body(std::move(b)) { }

  private:
    friend IntrusiveLinkedList<IfBody>;
    IfBody* next{ nullptr };
  };
  IntrusiveLinkedList<IfBody> ifBodies{ };
  IntrusiveLinkedList<PapyrusStatement> elseStatements{ };

  explicit PapyrusIfStatement(CapricaFileLocation loc) : PapyrusStatement(loc) { }
  PapyrusIfStatement(const PapyrusIfStatement&) = delete;
  virtual ~PapyrusIfStatement() override = default;

  virtual bool buildCFG(PapyrusCFG& cfg) const override {
    bool isTerminal = true;

    for (auto p : ifBodies) {
      cfg.addLeaf();
      isTerminal = cfg.processStatements(p->body) && isTerminal;
    }

    if (elseStatements.size()) {
      cfg.addLeaf();
      isTerminal = cfg.processStatements(elseStatements) && isTerminal;
    }

    if (isTerminal)
      cfg.terminateNode(PapyrusControlFlowNodeEdgeType::Children);
    else
      cfg.createSibling();
    return isTerminal;
  }

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    pex::PexLabel* afterAll;
    bldr >> afterAll;
    pex::PexLabel* nextCondition{ nullptr };
    for (auto& ifBody : ifBodies) {
      if (nextCondition)
        bldr << nextCondition;
      bldr >> nextCondition;
      auto lVal = ifBody->condition->generateLoad(file, bldr);
      bldr << location;
      bldr << op::jmpf{ lVal, nextCondition };
      for (auto s : ifBody->body)
        s->buildPex(file, bldr);
      bldr << location;
      bldr << op::jmp{ afterAll };
    }

    bldr << nextCondition;
    for (auto s : elseStatements)
      s->buildPex(file, bldr);
    bldr << afterAll;
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    for (auto& i : ifBodies) {
      i->condition->semantic(ctx);
      ctx->checkForPoison(i->condition);
      i->condition = ctx->coerceExpression(i->condition, PapyrusType::Bool(i->condition->location));
      ctx->pushLocalVariableScope();
      if (conf::Papyrus::game == GameID::Skyrim) {
          for (auto s: i->body) {
            s->semantic_skyrim_first_pass(ctx);
          }
      }
      for (auto s : i->body)
        s->semantic(ctx);
      ctx->popLocalVariableScope();
    }
    ctx->pushLocalVariableScope();
    if (conf::Papyrus::game == GameID::Skyrim) {
      for (auto s : elseStatements) {
        s->semantic_skyrim_first_pass(ctx);
      }
    }
    for (auto s : elseStatements)
      s->semantic(ctx);
    ctx->popLocalVariableScope();
  }

  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);

    for (auto i : ifBodies) {
      for (auto s : i->body)
        s->visit(visitor);
    }
    for (auto s : elseStatements)
      s->visit(visitor);
  }
};

}}}
