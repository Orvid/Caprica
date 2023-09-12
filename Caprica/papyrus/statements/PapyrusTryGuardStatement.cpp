#include <papyrus/statements/PapyrusTryGuardStatement.h>


namespace caprica { namespace papyrus { namespace statements {


struct PapyrusTryGuardStatementBodyVisitor : public PapyrusSelectiveStatementVisitor {
    const PapyrusTryGuardStatement *m_ThisTryGuardStatement{nullptr};
    IntrusiveLinkedList<PapyrusReturnStatement> m_ReturnStatements{};
    bool m_InvalidNestedLocks{false};
    PapyrusTryGuardStatementBodyVisitor(const PapyrusTryGuardStatement *thisLockStatement) : m_ThisTryGuardStatement(thisLockStatement) {}
    void visit(PapyrusTryGuardStatement *tls) override{
      m_InvalidNestedLocks = true;
// TODO: Starfield, verify: Fairly certain that nested locks are not allowed due to statements in the binary, need to verify once CK comes out
//      for (auto s: tls->lockParams) {
//        assert(m_ThisTryGuardStatement);
//        for (auto p : m_ThisTryGuardStatement->lockParams) {
//          // TODO: Check against the guard pointer?
//          if (s->name == p->name) {
//            m_InvalidNestedLocks = true;
//          }
//        }
//      }
    }
};

void PapyrusTryGuardStatement::semantic(PapyrusResolutionContext *ctx) {
  for (auto lockparam : lockParams)
    lockparam->semantic(ctx);
  ctx->pushLocalVariableScope();
  for (auto s: body) {
    s->semantic(ctx);
  }
  ctx->popLocalVariableScope();
  auto visitor = PapyrusTryGuardStatementBodyVisitor(this);
  for (auto s: body) {
    s->visit(visitor);
  }
  if (visitor.m_InvalidNestedLocks) {
    // TODO: Starfield, verify: Fairly certain that nested locks are not allowed due to statements in the binary, need to verify once CK comes out
    ctx->reportingContext.fatal(location, "Nested locks are not allowed");
  }
}

}}}

