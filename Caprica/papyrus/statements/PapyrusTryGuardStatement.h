#pragma once

#include <common/IntrusiveLinkedList.h>

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>
#include <papyrus/PapyrusLockParameter.h>
#include <papyrus/statements/PapyrusStatementVisitor.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>


namespace caprica { namespace papyrus { namespace statements {


struct PapyrusTryGuardStatement final : public PapyrusStatement
{
  IntrusiveLinkedList<PapyrusStatement> body{ };
  IntrusiveLinkedList<PapyrusLockParameter> lockParams{};
  explicit PapyrusTryGuardStatement(CapricaFileLocation loc) : PapyrusStatement(loc) { }
  PapyrusTryGuardStatement(const PapyrusTryGuardStatement&) = delete;
  virtual ~PapyrusTryGuardStatement() override = default;

  virtual bool buildCFG(PapyrusCFG& cfg) const override {
    bool isTerminal = true;

    cfg.addLeaf();
    isTerminal = cfg.processStatements(body) && isTerminal;

    if (isTerminal)
      cfg.terminateNode(PapyrusControlFlowNodeEdgeType::Children);
    else
      cfg.createSibling();
    return isTerminal;
  }

  // TODO: Not sure if this is at all correct
  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    IntrusiveLinkedList<pex::IntrusivePexValue> args;
    for (auto guard : lockParams){
      args.push_back(file->alloc->make<pex::IntrusivePexValue>(pex::PexValue(file->getString(guard->name))));
    }
    pex::PexLabel* afterAll;
    bldr >> afterAll;

    auto tlTemp = bldr.allocTemp(PapyrusType::Bool(location));
    bldr << location;
    bldr << op::trylockguards{ tlTemp, std::move(args) };
    bldr << op::jmpf( tlTemp , afterAll);
    bldr.pushLockScope(std::move(args));
    for (auto s : body) {
      s->buildPex(file, bldr);
    }
    bldr.popLockScope();
    bldr << location;
    bldr << op::unlockguards{ std::move(args) };
    bldr << afterAll;
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override;

  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);
    for (auto s : body)
      s->visit(visitor);
  }
};

}}}
