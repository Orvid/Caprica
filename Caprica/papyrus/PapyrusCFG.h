#pragma once

#include <stack>

#include <common/allocators/ChainedPool.h>
#include <common/CapricaReportingContext.h>
#include <common/IntrusiveLinkedList.h>
#include <common/IntrusiveStack.h>

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
  IntrusiveLinkedList<PapyrusControlFlowNode> children{ };
  PapyrusControlFlowNode* nextSibling{ nullptr };

  PapyrusControlFlowNode(int i) : id(i) { }
  ~PapyrusControlFlowNode() = default;

  void dumpNode(int currentDepth);

private:
  friend IntrusiveLinkedList<PapyrusControlFlowNode>;
  PapyrusControlFlowNode* next{ nullptr };
  friend IntrusiveStack<PapyrusControlFlowNode>;
  PapyrusControlFlowNode* nextInStack{ nullptr };
};

struct PapyrusCFG final
{
  CapricaReportingContext& reportingContext;
  PapyrusControlFlowNode* root{ nullptr };

  PapyrusCFG(CapricaReportingContext& repCtx) : reportingContext(repCtx) {
    root = alloc.make<PapyrusControlFlowNode>(nextNodeID++);
    nodeStack.push(root);
  }
  ~PapyrusCFG() = default;

  bool processStatements(const IntrusiveLinkedList<statements::PapyrusStatement>& stmts);

  bool processCommonLoopBody(const IntrusiveLinkedList<statements::PapyrusStatement>& stmts) {
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
    // We don't do anything here currently.
  }

  void terminateNode(PapyrusControlFlowNodeEdgeType tp) {
    nodeStack.top()->edgeType = tp;
    nodeStack.pop();
  }

  void addLeaf() {
    auto n = alloc.make<PapyrusControlFlowNode>(nextNodeID++);
    nodeStack.top()->children.push_back(n);
    nodeStack.push(n);
  }

  void createSibling() {
    auto n = alloc.make<PapyrusControlFlowNode>(nextNodeID++);
    nodeStack.top()->nextSibling = n;
    nodeStack.pop();
    nodeStack.push(n);
  }

  void pushBreakTerminal() {
    breakTargetStack.push(alloc.make<BreakTarget>());
  }

  void markBreakTerminal() {
    breakTargetStack.top()->isBreakTarget = true;
  }

  bool popBreakTerminal() {
    bool b = breakTargetStack.top()->isBreakTarget;
    breakTargetStack.pop();
    return b;
  }

  void dumpGraph() {
    root->dumpNode(0);
  }

private:
  struct BreakTarget final
  {
    bool isBreakTarget{ false };

  private:
    friend IntrusiveStack<BreakTarget>;
    BreakTarget* nextInStack{ nullptr };
  };

  allocators::ChainedPool alloc{ 4 * 1024 };
  int nextNodeID{ 0 };
  IntrusiveStack<PapyrusControlFlowNode> nodeStack{ };
  IntrusiveStack<BreakTarget> breakTargetStack{ };
};

}}
