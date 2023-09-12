#include <papyrus/statements/PapyrusLockStatement.h>

namespace caprica { namespace papyrus { namespace statements {
struct PapyrusLockStatementBodyVisitor : public PapyrusSelectiveStatementVisitor {
    const PapyrusLockStatement *m_ThisLockStatement{nullptr};
    bool m_InvalidNestedLocks{false};

    PapyrusLockStatementBodyVisitor(const PapyrusLockStatement *thisLockStatement)
            : m_ThisLockStatement(thisLockStatement) {}

    virtual void visit(PapyrusLockStatement *ls) override {
      m_InvalidNestedLocks = true;
// TODO: Starfield, verify: Fairly certain that nested locks are not allowed due to statements in the binary, need to verify once CK comes out
//      for (auto s: ls->lockParams) {
//        assert(m_ThisLockStatement);
//        for (auto p: m_ThisLockStatement->lockParams) {
//          // TODO: Check against the guard pointer?
//          if (s->name == p->name) {
//            m_InvalidNestedLocks = true;
//          }
//        }
//      }
    }
};

void PapyrusLockStatement::semantic(PapyrusResolutionContext *ctx) {
  for (auto lockparam: lockParams)
    lockparam->semantic(ctx);
  ctx->pushLocalVariableScope();
  for (auto s: body) {
    s->semantic(ctx);
  }
  ctx->popLocalVariableScope();
  auto visitor = PapyrusLockStatementBodyVisitor(this);
  for (auto s: body) {
    s->visit(visitor);
  }
  if (visitor.m_InvalidNestedLocks) {
    // TODO: Starfield, verify: Fairly certain that nested locks are not allowed due to statements in the binary, need to verify once CK comes out
    ctx->reportingContext.fatal(location, "Nested locks are not allowed");
  }
}
}}}