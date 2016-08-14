#pragma once

#include <cstring>
#include <vector>
#include <unordered_set>

#include <common/allocators/ChainedPool.h>
#include <common/CapricaFileLocation.h>
#include <common/CaselessStringComparer.h>
#include <common/identifier_ref.h>
#include <common/IntrusiveLinkedList.h>
#include <common/IntrusiveStack.h>

namespace caprica { namespace papyrus { struct PapyrusResolutionContext; } }

#include <papyrus/PapyrusIdentifier.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusValue.h>

namespace caprica { namespace papyrus {

struct PapyrusCompilationNode;
struct PapyrusCustomEvent;
struct PapyrusFunction;
struct PapyrusObject;
struct PapyrusScript;
struct PapyrusState;

namespace expressions { struct PapyrusExpression; }
namespace statements { struct PapyrusDeclareStatement; }

// This is the core of the semantic operations. It handles resolving everything,
// the coercion and casting rules, and general utility methods for use in the
// semantic pass.
struct PapyrusResolutionContext final
{
  CapricaReportingContext& reportingContext;
  allocators::ChainedPool* allocator{ nullptr };
  const PapyrusScript* script{ nullptr };
  const PapyrusObject* object{ nullptr };
  const PapyrusState* state{ nullptr };
  const PapyrusFunction* function{ nullptr };
  // If true, we're resolving a tree generated from
  // a pex file.
  bool isPexResolution{ false };

  void addImport(const CapricaFileLocation& location, const identifier_ref& import);
  void clearImports() { importedNodes.clear(); }

  static bool isObjectSomeParentOf(const PapyrusObject* child, const PapyrusObject* parent);
  static bool canExplicitlyCast(const PapyrusType& src, const PapyrusType& dest);
  static bool canImplicitlyCoerce(const PapyrusType& src, const PapyrusType& dest);
  static bool canImplicitlyCoerceExpression(expressions::PapyrusExpression* expr, const PapyrusType& target);
  expressions::PapyrusExpression* coerceExpression(expressions::PapyrusExpression* expr, const PapyrusType& target) const;
  PapyrusValue coerceDefaultValue(const PapyrusValue& val, const PapyrusType& target) const;

  void checkForPoison(const expressions::PapyrusExpression* expr) const;
  void checkForPoison(const PapyrusType& type) const;

  void pushLocalVariableScope() { localVariableScopeStack.push(allocator->make<LocalScopeStackNode>()); }
  void popLocalVariableScope() { localVariableScopeStack.pop(); }

  bool canBreak() const { return currentBreakScopeDepth > 0; }
  void pushBreakScope() { currentBreakScopeDepth++; }
  void popBreakScope() { currentBreakScopeDepth--; }

  bool canContinue() const {
    return currentContinueScopeDepth > 0;
  }
  void pushBreakContinueScope() {
    currentBreakScopeDepth++;
    currentContinueScopeDepth++;
  }
  void popBreakContinueScope() {
    currentBreakScopeDepth--;
    currentContinueScopeDepth--;
  }

  void addLocalVariable(statements::PapyrusDeclareStatement* ident);

  const PapyrusFunction* tryResolveEvent(const PapyrusObject* parentObj, const identifier_ref& name) const;
  const PapyrusCustomEvent* tryResolveCustomEvent(const PapyrusObject* parentObj, const identifier_ref& name) const;
  const PapyrusState* tryResolveState(const identifier_ref& name, const PapyrusObject* parentObj = nullptr) const;
  PapyrusType resolveType(PapyrusType tp, bool lazy = false);
  PapyrusIdentifier resolveIdentifier(const PapyrusIdentifier& ident) const;
  PapyrusIdentifier tryResolveIdentifier(const PapyrusIdentifier& ident) const;
  PapyrusIdentifier resolveMemberIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const;
  PapyrusIdentifier tryResolveMemberIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const;
  PapyrusIdentifier resolveFunctionIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident, bool wantGlobal = false) const;
  PapyrusIdentifier tryResolveFunctionIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident, bool wantGlobal = false) const;

  template<typename T>
  void ensureNamesAreUnique(const IntrusiveLinkedList<T>& nameset, const char* typeOfName) {
    // If there's nothing in it, or only one thing,
    // it will always be unique.
    if (nameset.size() > 1) {
      caseless_unordered_identifier_ref_set foundNames{ };
      foundNames.reserve(nameset.size());
      for (auto member : nameset) {
        // TODO: Output location of first name.
        auto f = foundNames.find(member->name);
        if (f != foundNames.end()) {
          reportingContext.error(member->location, "A %s named '%s' was already defined in this scope.", typeOfName, member->name.to_string().c_str());
        } else {
          foundNames.insert(member->name);
        }
      }
    }
  }

  explicit PapyrusResolutionContext(CapricaReportingContext& repCtx) : reportingContext(repCtx) { }
  PapyrusResolutionContext(const PapyrusResolutionContext&) = delete;
  ~PapyrusResolutionContext() = default;
private:
  struct LocalScopeVariableNode final
  {
    identifier_ref name{ };
    statements::PapyrusDeclareStatement* declareStatement{ nullptr };

    LocalScopeVariableNode(const identifier_ref& nm, statements::PapyrusDeclareStatement* decl) : name(nm), declareStatement(decl) { }
  private:
    friend IntrusiveLinkedList<LocalScopeVariableNode>;
    LocalScopeVariableNode* next{ nullptr };
  };
  struct LocalScopeStackNode final
  {
    IntrusiveLinkedList<LocalScopeVariableNode> locals{ };

  private:
    friend IntrusiveStack<LocalScopeStackNode>;
    LocalScopeStackNode* nextInStack{ nullptr };
  };
  IntrusiveStack<LocalScopeStackNode> localVariableScopeStack{ };
  std::vector<PapyrusCompilationNode*> importedNodes{ };
  size_t currentBreakScopeDepth{ 0 };
  size_t currentContinueScopeDepth{ 0 };
};

}}
