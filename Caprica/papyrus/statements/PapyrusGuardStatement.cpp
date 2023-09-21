#include <papyrus/statements/PapyrusGuardStatement.h>

namespace caprica { namespace papyrus { namespace statements {
struct PapyrusGuardStatementBodyVisitor : public PapyrusSelectiveStatementVisitor {
    const PapyrusGuardStatement *m_ThisGuardStatement{nullptr};
    bool m_InvalidNestedLocks{false};

    PapyrusGuardStatementBodyVisitor(const PapyrusGuardStatement *thisLockStatement)
            : m_ThisGuardStatement(thisLockStatement) {}

    virtual void visit(PapyrusGuardStatement *ls) override {
// TODO: Starfield, verify: Scripts do in fact have nested lock guards; need to verify once CK comes out
      for (auto s: ls->lockParams) {
        assert(m_ThisGuardStatement);
        for (auto p: m_ThisGuardStatement->lockParams) {
          // TODO: Check against the guard pointer?
          if (s->name == p->name) {
            m_InvalidNestedLocks = true;
          }
        }
      }
    }
};

void PapyrusGuardStatement::semantic(PapyrusResolutionContext *ctx) {
  for (auto lockparam: lockParams)
    lockparam->semantic(ctx);
  ctx->pushLocalVariableScope();
  for (auto s: body) {
    s->semantic(ctx);
  }
  ctx->popLocalVariableScope();
  auto visitor = PapyrusGuardStatementBodyVisitor(this);
  for (auto s: body) {
    s->visit(visitor);
  }
  if (visitor.m_InvalidNestedLocks) {
    // TODO: Starfield, verify
    ctx->reportingContext.fatal(location, "Invalid nested lock found!");
  }
}
}}}