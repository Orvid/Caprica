#pragma once

#include <common/IntrusiveLinkedList.h>

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusDeclareStatement.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusForEachStatement final : public PapyrusStatement
{
  PapyrusDeclareStatement* declareStatement{ nullptr };
  expressions::PapyrusExpression* expressionToIterate{ nullptr };
  IntrusiveLinkedList<PapyrusStatement> body{ };
  PapyrusIdentifier* getCountIdentifier{ nullptr };
  PapyrusIdentifier* getAtIdentifier{ nullptr };

  explicit PapyrusForEachStatement(CapricaFileLocation loc) : PapyrusStatement(loc) { }
  PapyrusForEachStatement(const PapyrusForEachStatement&) = delete;
  virtual ~PapyrusForEachStatement() override = default;

  virtual bool buildCFG(PapyrusCFG& cfg) const override {
    cfg.appendStatement(declareStatement);
    return cfg.processCommonLoopBody(body);
  }
  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override;
  virtual void semantic(PapyrusResolutionContext* ctx) override;
  virtual void visit(PapyrusStatementVisitor& visitor) override {
    visitor.visit(this);

    declareStatement->visit(visitor);
    for (auto s : body)
      s->visit(visitor);
  }
};

}}}
