#pragma once

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusAssignStatement;
struct PapyrusDeclareStatement;
struct PapyrusExpressionStatement;
struct PapyrusIfStatement;
struct PapyrusReturnStatement;
struct PapyrusWhileStatement;

struct PapyrusStatementVisitor abstract
{
  PapyrusStatementVisitor() = default;
  virtual ~PapyrusStatementVisitor() = default;

  virtual void visit(PapyrusAssignStatement* s) { }
  virtual void visit(PapyrusDeclareStatement* s) { }
  virtual void visit(PapyrusExpressionStatement* s) { }
  virtual void visit(PapyrusIfStatement* s) { }
  virtual void visit(PapyrusReturnStatement* s) { }
  virtual void visit(PapyrusWhileStatement* s) { }
};

}}}
