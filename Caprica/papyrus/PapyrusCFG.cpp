#include <papyrus/PapyrusCFG.h>

#include <iostream>

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
#include <papyrus/statements/PapyrusLockStatement.h>
#include <papyrus/statements/PapyrusReturnStatement.h>
#include <papyrus/statements/PapyrusSwitchStatement.h>
#include <papyrus/statements/PapyrusWhileStatement.h>

namespace caprica { namespace papyrus {

void PapyrusControlFlowNode::dumpNode(int currentDepth) {
  const auto writeIndent = [](int depth) {
    for (int i = 0; i < depth; i++)
      std::cout << "  ";
  };

  writeIndent(currentDepth);
  std::cout << "Node " << id << " " << std::endl;

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

bool PapyrusCFG::processStatements(const IntrusiveLinkedList<statements::PapyrusStatement>& stmts) {
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
