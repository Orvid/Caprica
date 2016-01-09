#pragma once

#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <unordered_set>

#include <boost/algorithm/string/case_conv.hpp>

namespace caprica { namespace papyrus { struct PapyrusResolutionContext; } }

#include <common/CapricaFileLocation.h>
#include <common/CaselessStringComparer.h>
#include <papyrus/PapyrusIdentifier.h>
#include <papyrus/PapyrusType.h>

namespace caprica { namespace papyrus {

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
  const PapyrusScript* script{ nullptr };
  const PapyrusObject* object{ nullptr };
  const PapyrusState* state{ nullptr };
  const PapyrusFunction* function{ nullptr };
  // This is only true if the resolution being done
  // isn't being done to be able to emit the pex.
  // This means that anything that can't be referenced
  // by an external script is free to be removed from
  // the object, and doesn't need to be resolved.
  bool resolvingReferenceScript{ false };
  // If true, we're resolving a tree generated from
  // a pex file.
  bool isPexResolution{ false };

  void addImport(const CapricaFileLocation& location, const std::string& import);

  static bool isObjectSomeParentOf(const PapyrusObject* child, const PapyrusObject* parent);
  static bool canExplicitlyCast(const PapyrusType& src, const PapyrusType& dest);
  static bool canImplicitlyCoerce(const PapyrusType& src, const PapyrusType& dest);
  static bool canImplicitlyCoerceExpression(expressions::PapyrusExpression* expr, const PapyrusType& target, bool& needsCast);
  static expressions::PapyrusExpression* coerceExpression(expressions::PapyrusExpression* expr, const PapyrusType& target);

  void pushLocalVariableScope() {
    localVariableScopeStack.push_back({ });
  }

  void popLocalVariableScope() {
    localVariableScopeStack.pop_back();
  }

  void addLocalVariable(statements::PapyrusDeclareStatement* ident);

  PapyrusType resolveType(PapyrusType tp);
  PapyrusIdentifier resolveIdentifier(const PapyrusIdentifier& ident) const;
  PapyrusIdentifier tryResolveIdentifier(const PapyrusIdentifier& ident) const;
  PapyrusIdentifier resolveMemberIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const;
  PapyrusIdentifier tryResolveMemberIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const;
  PapyrusIdentifier resolveFunctionIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const;

  template<typename T>
  static void ensureNamesAreUnique(const std::vector<T*>& nameset, const std::string& typeOfName) {
    // TODO: This has a short enough lifetime that an
    // std::set might actually outperform this.
    std::unordered_set<std::string, CaselessStringHasher> foundNames{ };
    foundNames.reserve(nameset.size());
    for (auto member : nameset) {
      // TODO: Output location of first name.
      auto f = foundNames.find(member->name);
      if (f != foundNames.end()) {
        CapricaError::error(member->location, "A %s named '%s' was already defined in this scope.", typeOfName.c_str(), member->name.c_str());
      } else {
        foundNames.insert(member->name);
      }
    }
  }

  PapyrusResolutionContext() = default;
  ~PapyrusResolutionContext() = default;
private:
  std::vector<std::map<std::string, statements::PapyrusDeclareStatement*, CaselessStringComparer>> localVariableScopeStack{ };
  std::vector<PapyrusScript*> importedScripts{ };

  PapyrusScript* loadScript(const std::string& name);
};

}}
