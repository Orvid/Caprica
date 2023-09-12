#include <papyrus/statements/PapyrusTryLockStatement.h>


namespace caprica { namespace papyrus { namespace statements {


struct PapyrusTryLockStatementBodyVisitor : public PapyrusSelectiveStatementVisitor {
    const PapyrusTryLockStatement *m_ThisTryLockStatement{nullptr};
    IntrusiveLinkedList<PapyrusReturnStatement> m_ReturnStatements{};
    bool m_InvalidNestedLocks{false};
    PapyrusTryLockStatementBodyVisitor(const PapyrusTryLockStatement *thisLockStatement) : m_ThisTryLockStatement(thisLockStatement) {}
    virtual void visit(PapyrusTryLockStatement *tls) override{
      m_InvalidNestedLocks = true;
// TODO: Starfield, verify: Fairly certain that nested locks are not allowed due to statements in the binary, need to verify once CK comes out
//      for (auto s: tls->lockParams) {
//        assert(m_ThisTryLockStatement);
//        for (auto p : m_ThisTryLockStatement->lockParams) {
//          // TODO: Check against the guard pointer?
//          if (s->name == p->name) {
//            m_InvalidNestedLocks = true;
//          }
//        }
//      }
    }
};

void PapyrusTryLockStatement::semantic(PapyrusResolutionContext *ctx) {
  for (auto lockparam : lockParams)
    lockparam->semantic(ctx);
  ctx->pushLocalVariableScope();
  for (auto s: body) {
    s->semantic(ctx);
  }
  ctx->popLocalVariableScope();
  auto visitor = PapyrusTryLockStatementBodyVisitor(this);
  for (auto s: body) {
    s->visit(visitor);
  }
  if (visitor.m_InvalidNestedLocks) {
    // TODO: Starfield, verify: Fairly certain that nested locks are not allowed due to statements in the binary, need to verify once CK comes out
    ctx->reportingContext.fatal(location, "Nested locks are not allowed");
  }
}

}}}

