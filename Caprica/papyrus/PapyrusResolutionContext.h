#pragma once

#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <boost/algorithm/string/case_conv.hpp>

namespace caprica { namespace papyrus { struct PapyrusResolutionContext; } }

#include <common/CapricaFileLocation.h>
#include <common/CaselessStringComparer.h>
#include <papyrus/PapyrusIdentifier.h>
#include <papyrus/PapyrusType.h>


namespace caprica { namespace papyrus {

struct PapyrusFunction;
struct PapyrusObject;
struct PapyrusProperty;
struct PapyrusPropertyGroup;
struct PapyrusScript;
struct PapyrusState;
struct PapyrusStruct;

namespace expressions { struct PapyrusExpression; }

struct PapyrusResolutionContext final
{
  const PapyrusScript* script{ nullptr };
  const PapyrusObject* object{ nullptr };
  const PapyrusStruct* struc{ nullptr };
  const PapyrusProperty* prop{ nullptr };
  const PapyrusPropertyGroup* propGroup{ nullptr };
  const PapyrusState* state{ nullptr };
  const PapyrusFunction* function{ nullptr };
  // This is only true if the resolution being done
  // isn't being done to be able to emit the pex.
  // This means that function bodies shouldn't be
  // resolved.
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

  void pushIdentifierScope() {
    identifierStack.push_back({ });
  }

  void popIdentifierScope() {
    identifierStack.pop_back();
  }

  PapyrusType resolveType(PapyrusType tp);
  void addIdentifier(const PapyrusIdentifier& ident);
  PapyrusIdentifier resolveIdentifier(const PapyrusIdentifier& ident) const;
  PapyrusIdentifier tryResolveIdentifier(const PapyrusIdentifier& ident) const;
  PapyrusIdentifier resolveMemberIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const;
  PapyrusIdentifier tryResolveMemberIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const;
  PapyrusIdentifier resolveFunctionIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const;

  PapyrusResolutionContext() = default;
  ~PapyrusResolutionContext() = default;
private:
  std::vector<std::map<std::string, PapyrusIdentifier, CaselessStringComparer>> identifierStack{ };
  std::vector<PapyrusScript*> importedScripts{ };

  PapyrusScript* loadScript(const std::string& name);
};

}}
