#pragma once

#include <iostream>
#include <stack>
#include <vector>

#include <common/CapricaReportingContext.h>

namespace caprica { namespace papyrus {

namespace expressions { struct PapyrusExpression; }
namespace statements { struct PapyrusStatement; }

enum class PapyrusControlFlowNodeEdgeType
{
  None,
  Continue,
  Break,
  Return,
  /**
   * All children of this node are edges.
   */
  Children,
};

struct PapyrusControlFlowNode final
{
  int id{ };
  PapyrusControlFlowNodeEdgeType edgeType{ PapyrusControlFlowNodeEdgeType::None };
  //std::vector<const statements::PapyrusStatement*> statements{ };
  std::vector<PapyrusControlFlowNode*> children{ };
  PapyrusControlFlowNode* nextSibling{ nullptr };

  PapyrusControlFlowNode(int i) : id(i) { }
  ~PapyrusControlFlowNode() {
    for (auto c : children)
      delete c;
    if (nextSibling)
      delete nextSibling;
  }

  void dumpNode(int currentDepth);
};

struct PapyrusCFG final
{
  CapricaReportingContext& reportingContext;
  PapyrusControlFlowNode* root{ nullptr };

  PapyrusCFG(CapricaReportingContext& repCtx) : reportingContext(repCtx) {
    root = new PapyrusControlFlowNode(nextNodeID++);
    nodeStack.push(root);
  }
  ~PapyrusCFG() {
    if (root)
      delete root;
  }

  bool processStatements(const std::vector<statements::PapyrusStatement*>& stmts);

  bool processCommonLoopBody(const std::vector<statements::PapyrusStatement*>& stmts) {
    pushBreakTerminal();
    addLeaf();
    bool wasTerminal = processStatements(stmts);
    bool isTerminal = !popBreakTerminal() && wasTerminal;
    if (isTerminal)
      terminateNode(PapyrusControlFlowNodeEdgeType::Children);
    else
      createSibling();
    return isTerminal;
  }

  void appendStatement(const statements::PapyrusStatement* stmt) {
    //nodeStack.top()->statements.push_back(stmt);
  }

  void terminateNode(PapyrusControlFlowNodeEdgeType tp) {
    nodeStack.top()->edgeType = tp;
    nodeStack.pop();
  }

  void addLeaf() {
    auto n = new PapyrusControlFlowNode(nextNodeID++);
    nodeStack.top()->children.push_back(n);
    nodeStack.push(n);
  }

  void createSibling() {
    auto n = new PapyrusControlFlowNode(nextNodeID++);
    nodeStack.top()->nextSibling = n;
    nodeStack.pop();
    nodeStack.push(n);
  }

  void pushBreakTerminal() {
    breakTargetStack.push(false);
  }

  void markBreakTerminal() {
    breakTargetStack.top() = true;
  }

  bool popBreakTerminal() {
    bool b = breakTargetStack.top();
    breakTargetStack.pop();
    return b;
  }

  void dumpGraph() {
    root->dumpNode(0);
  }

private:
  int nextNodeID{ 0 };
  std::stack<PapyrusControlFlowNode*> nodeStack{ };
  std::stack<bool> breakTargetStack{ };
};

}}
