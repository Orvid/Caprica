#pragma once

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusAssignStatement;
struct PapyrusBreakStatement;
struct PapyrusContinueStatement;
struct PapyrusDeclareStatement;
struct PapyrusDoWhileStatement;
struct PapyrusExpressionStatement;
struct PapyrusForStatement;
struct PapyrusForEachStatement;
struct PapyrusIfStatement;
struct PapyrusGuardStatement;
struct PapyrusReturnStatement;
struct PapyrusSwitchStatement;
struct PapyrusTryGuardStatement;
struct PapyrusWhileStatement;

struct PapyrusStatementVisitor abstract
{
  explicit PapyrusStatementVisitor() = default;
  PapyrusStatementVisitor(const PapyrusStatementVisitor&) = delete;
  virtual ~PapyrusStatementVisitor() = default;

  virtual void visit(PapyrusAssignStatement* s) = 0;
  virtual void visit(PapyrusBreakStatement* s) = 0;
  virtual void visit(PapyrusContinueStatement* s) = 0;
  virtual void visit(PapyrusDeclareStatement* s) = 0;
  virtual void visit(PapyrusDoWhileStatement* s) = 0;
  virtual void visit(PapyrusExpressionStatement* s) = 0;
  virtual void visit(PapyrusForStatement* s) = 0;
  virtual void visit(PapyrusForEachStatement* s) = 0;
  virtual void visit(PapyrusIfStatement* s) = 0;
  virtual void visit(PapyrusGuardStatement* s) = 0;
  virtual void visit(PapyrusReturnStatement* s) = 0;
  virtual void visit(PapyrusSwitchStatement* s) = 0;
  virtual void visit(PapyrusTryGuardStatement* s) = 0;
  virtual void visit(PapyrusWhileStatement* s) = 0;
};

struct PapyrusSelectiveStatementVisitor abstract : public PapyrusStatementVisitor
{
  explicit PapyrusSelectiveStatementVisitor() = default;
  PapyrusSelectiveStatementVisitor(const PapyrusSelectiveStatementVisitor&) = delete;
  virtual ~PapyrusSelectiveStatementVisitor() = default;

  virtual void visit(PapyrusAssignStatement*) override { }
  virtual void visit(PapyrusBreakStatement*) override { }
  virtual void visit(PapyrusContinueStatement*) override { }
  virtual void visit(PapyrusDeclareStatement*) override { }
  virtual void visit(PapyrusDoWhileStatement*) override { }
  virtual void visit(PapyrusExpressionStatement*) override { }
  virtual void visit(PapyrusForStatement*) override { }
  virtual void visit(PapyrusForEachStatement*) override { }
  virtual void visit(PapyrusIfStatement*) override { }
  virtual void visit(PapyrusGuardStatement*) override { }
  virtual void visit(PapyrusReturnStatement*) override { }
  virtual void visit(PapyrusSwitchStatement*) override { }
  virtual void visit(PapyrusTryGuardStatement*) override { }
  virtual void visit(PapyrusWhileStatement*) override { }
};

}}}
