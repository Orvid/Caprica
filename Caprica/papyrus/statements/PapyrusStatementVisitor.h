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
  PapyrusStatementVisitor() = default;
  virtual ~PapyrusStatementVisitor() = default;

  virtual void visit(PapyrusAssignStatement* s) { }
  virtual void visit(PapyrusBreakStatement* s) { }
  virtual void visit(PapyrusContinueStatement* s) { }
  virtual void visit(PapyrusDeclareStatement* s) { }
  virtual void visit(PapyrusDoWhileStatement* s) { }
  virtual void visit(PapyrusExpressionStatement* s) { }
  virtual void visit(PapyrusForStatement* s) { }
  virtual void visit(PapyrusForEachStatement* s) { }
  virtual void visit(PapyrusIfStatement* s) { }
  virtual void visit(PapyrusReturnStatement* s) { }
  virtual void visit(PapyrusSwitchStatement* s) { }
  virtual void visit(PapyrusWhileStatement* s) { }
};

}}}
