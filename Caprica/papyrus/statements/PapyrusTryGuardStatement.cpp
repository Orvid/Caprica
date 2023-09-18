#include <papyrus/statements/PapyrusTryGuardStatement.h>


namespace caprica { namespace papyrus { namespace statements {


struct PapyrusTryGuardStatementBodyVisitor : public PapyrusSelectiveStatementVisitor {
    const PapyrusTryGuardStatement *m_ThisTryGuardStatement{nullptr};
    IntrusiveLinkedList<PapyrusReturnStatement> m_ReturnStatements{};
    bool m_InvalidNestedLocks{false};
    PapyrusTryGuardStatementBodyVisitor(const PapyrusTryGuardStatement *thisLockStatement) : m_ThisTryGuardStatement(thisLockStatement) {}
    void visit(PapyrusTryGuardStatement *tls) override{
      m_InvalidNestedLocks = true;
      // TODO: Starfield, verify: Scripts do in fact have nested lock guards; need to verify once CK comes out
      for (auto s: tls->lockParams) {
        assert(m_ThisTryGuardStatement);
        for (auto p : m_ThisTryGuardStatement->lockParams) {
          // TODO: Check against the guard pointer?
          if (s->name == p->name) {
            m_InvalidNestedLocks = true;
          }
        }
      }
    }
};

void PapyrusTryGuardStatement::semantic(PapyrusResolutionContext *ctx) {
  if (conf::Papyrus::game != GameID::Starfield){
    ctx->reportingContext.fatal(location, "TryGuard statements are illegal < Starfield!");
  }
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
    // TODO: Starfield, verify
    ctx->reportingContext.fatal(location, "Invalid nested lock found!");
  }
}

}}}

