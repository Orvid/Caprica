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
struct PapyrusReturnStatement;
struct PapyrusSwitchStatement;
struct PapyrusWhileStatement;

struct PapyrusStatementVisitor abstract
{
  explicit PapyrusStatementVisitor() = default;
  PapyrusStatementVisitor(const PapyrusStatementVisitor&) = delete;
  virtual ~PapyrusStatementVisitor() = default;

  virtual void visit(PapyrusAssignStatement* s) abstract;
  virtual void visit(PapyrusBreakStatement* s) abstract;
  virtual void visit(PapyrusContinueStatement* s) abstract;
  virtual void visit(PapyrusDeclareStatement* s) abstract;
  virtual void visit(PapyrusDoWhileStatement* s) abstract;
  virtual void visit(PapyrusExpressionStatement* s) abstract;
  virtual void visit(PapyrusForStatement* s) abstract;
  virtual void visit(PapyrusForEachStatement* s) abstract;
  virtual void visit(PapyrusIfStatement* s) abstract;
  virtual void visit(PapyrusReturnStatement* s) abstract;
  virtual void visit(PapyrusSwitchStatement* s) abstract;
  virtual void visit(PapyrusWhileStatement* s) abstract;
};

struct PapyrusSelectiveStatementVisitor abstract : public PapyrusStatementVisitor
{
  explicit PapyrusSelectiveStatementVisitor() = default;
  PapyrusSelectiveStatementVisitor(const PapyrusSelectiveStatementVisitor&) = delete;
  virtual ~PapyrusSelectiveStatementVisitor() = default;

  virtual void visit(PapyrusAssignStatement* s) override { }
  virtual void visit(PapyrusBreakStatement* s) override { }
  virtual void visit(PapyrusContinueStatement* s) override { }
  virtual void visit(PapyrusDeclareStatement* s) override { }
  virtual void visit(PapyrusDoWhileStatement* s) override { }
  virtual void visit(PapyrusExpressionStatement* s) override { }
  virtual void visit(PapyrusForStatement* s) override { }
  virtual void visit(PapyrusForEachStatement* s) override { }
  virtual void visit(PapyrusIfStatement* s) override { }
  virtual void visit(PapyrusReturnStatement* s) override { }
  virtual void visit(PapyrusSwitchStatement* s) override { }
  virtual void visit(PapyrusWhileStatement* s) override { }
};

}}}
