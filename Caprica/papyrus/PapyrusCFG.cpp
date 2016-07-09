#include <papyrus/PapyrusCFG.h>

#include <common/CapricaReportingContext.h>

#include <papyrus/statements/PapyrusStatementVisitor.h>

#include <papyrus/expressions/PapyrusArrayIndexExpression.h>
#include <papyrus/expressions/PapyrusArrayLengthExpression.h>
#include <papyrus/expressions/PapyrusBinaryOpExpression.h>
#include <papyrus/expressions/PapyrusCastExpression.h>
#include <papyrus/expressions/PapyrusFunctionCallExpression.h>
#include <papyrus/expressions/PapyrusIdentifierExpression.h>
#include <papyrus/expressions/PapyrusIsExpression.h>
#include <papyrus/expressions/PapyrusLiteralExpression.h>
#include <papyrus/expressions/PapyrusMemberAccessExpression.h>
#include <papyrus/expressions/PapyrusNewArrayExpression.h>
#include <papyrus/expressions/PapyrusNewStructExpression.h>
#include <papyrus/expressions/PapyrusParentExpression.h>
#include <papyrus/expressions/PapyrusSelfExpression.h>
#include <papyrus/expressions/PapyrusUnaryOpExpression.h>

#include <papyrus/statements/PapyrusAssignStatement.h>
#include <papyrus/statements/PapyrusBreakStatement.h>
#include <papyrus/statements/PapyrusContinueStatement.h>
#include <papyrus/statements/PapyrusDeclareStatement.h>
#include <papyrus/statements/PapyrusDoWhileStatement.h>
#include <papyrus/statements/PapyrusExpressionStatement.h>
#include <papyrus/statements/PapyrusForStatement.h>
#include <papyrus/statements/PapyrusForEachStatement.h>
#include <papyrus/statements/PapyrusIfStatement.h>
#include <papyrus/statements/PapyrusReturnStatement.h>
#include <papyrus/statements/PapyrusSwitchStatement.h>
#include <papyrus/statements/PapyrusWhileStatement.h>

namespace caprica { namespace papyrus {

static void writeStatementType(const statements::PapyrusStatement* stmt) {
  struct NameDumpingStatementVisitor final : statements::PapyrusStatementVisitor
  {
    virtual void visit(statements::PapyrusAssignStatement* s) override {
      std::cout << "Assign";
    }
    virtual void visit(statements::PapyrusBreakStatement* s) override {
      std::cout << "Break";
    }
    virtual void visit(statements::PapyrusContinueStatement* s) override {
      std::cout << "Continue";
    }
    virtual void visit(statements::PapyrusDeclareStatement* s) override {
      std::cout << "Declare";
    }
    virtual void visit(statements::PapyrusDoWhileStatement* s) override {
      std::cout << "DoWhile";
    }
    virtual void visit(statements::PapyrusExpressionStatement* s) override {
      std::cout << "Expression";
    }
    virtual void visit(statements::PapyrusForStatement* s) override {
      std::cout << "For";
    }
    virtual void visit(statements::PapyrusForEachStatement* s) override {
      std::cout << "ForEach";
    }
    virtual void visit(statements::PapyrusIfStatement* s) override {
      std::cout << "If";
    }
    virtual void visit(statements::PapyrusReturnStatement* s) override {
      std::cout << "Return";
    }
    virtual void visit(statements::PapyrusSwitchStatement* s) override {
      std::cout << "Switch";
    }
    virtual void visit(statements::PapyrusWhileStatement* s) override {
      std::cout << "While";
    }
  } visitor;
  ((statements::PapyrusStatement*)stmt)->visit(visitor);
}

void PapyrusControlFlowNode::dumpNode(int currentDepth) {
  const auto writeIndent = [](int depth) {
    for (int i = 0; i < depth; i++)
      std::cout << "  ";
  };

  writeIndent(currentDepth);
  std::cout << "Node " << id << " " << std::endl;
  
  /*for (auto s : statements) {
    writeIndent(currentDepth + 1);
    writeStatementType(s);
    std::cout << std::endl;
  }*/

  if (edgeType != PapyrusControlFlowNodeEdgeType::None) {
    writeIndent(currentDepth + 1);
    std::cout << "Edge: " << [](PapyrusControlFlowNodeEdgeType edgeType) {
      switch (edgeType) {
        case PapyrusControlFlowNodeEdgeType::None:
          return "None";
        case PapyrusControlFlowNodeEdgeType::Break:
          return "Break";
        case PapyrusControlFlowNodeEdgeType::Continue:
          return "Continue";
        case PapyrusControlFlowNodeEdgeType::Return:
          return "Return";
        case PapyrusControlFlowNodeEdgeType::Children:
          return "Children";
      }
      CapricaReportingContext::logicalFatal("Unknown PapyrusControlFlowNodeEdgeType!");
    }(edgeType) << std::endl;
  }

  for (auto c : children) {
    c->dumpNode(currentDepth + 1);
  }

  if (nextSibling) {
    writeIndent(currentDepth - 1);
    std::cout << ">" << std::endl;
    nextSibling->dumpNode(currentDepth);
  }
}

bool PapyrusCFG::processStatements(const std::vector<statements::PapyrusStatement*>& stmts) {
  bool wasTerminal = false;
  for (auto s : stmts) {
    if (s->buildCFG(*this)) {
      wasTerminal = true;
      break;
    }
  }
  if (!wasTerminal)
    terminateNode(PapyrusControlFlowNodeEdgeType::None);
  return wasTerminal;
}

}}
