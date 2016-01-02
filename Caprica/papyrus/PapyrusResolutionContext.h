#pragma once

#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <boost/algorithm/string/case_conv.hpp>

namespace caprica { namespace papyrus { struct PapyrusResolutionContext; } }

#include <papyrus/PapyrusIdentifier.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/parser/PapyrusLexer.h>

namespace caprica { namespace papyrus {

struct PapyrusFunction;
struct PapyrusObject;
struct PapyrusProperty;
struct PapyrusPropertyGroup;
struct PapyrusScript;
struct PapyrusState;
struct PapyrusStruct;

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
  bool isExternalResolution{ false };

  void addImport(const parser::PapyrusFileLocation& location, const std::string& import);
  PapyrusType resolveType(PapyrusType tp);

  void ensureCastable(PapyrusType src, PapyrusType dest) {

  }

  void pushIdentifierScope() {
    identifierStack.push_back({ });
  }

  void popIdentifierScope() {
    identifierStack.pop_back();
  }

  void addIdentifier(const PapyrusIdentifier& ident) {
    // TODO: Ensure no parent scopes have the name.
    identifierStack.back().insert({ ident.name, ident });
  }

  PapyrusIdentifier resolveIdentifier(const PapyrusIdentifier& ident) const {
    auto id = tryResolveIdentifier(ident);
    if (id.type == PapyrusIdentifierType::Unresolved)
      CapricaError::fatal(ident.location, "Unresolved identifier '%s'!", ident.name.c_str());
    return id;
  }

  PapyrusIdentifier tryResolveIdentifier(const PapyrusIdentifier& ident) const;
  PapyrusIdentifier resolveMemberIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const;
  PapyrusIdentifier tryResolveMemberIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const;
  PapyrusIdentifier resolveFunctionIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const;

  PapyrusResolutionContext() = default;
  ~PapyrusResolutionContext() = default;
private:
  std::vector<std::map<std::string, PapyrusIdentifier, parser::CaselessStringComparer>> identifierStack{ };
  std::vector<PapyrusScript*> importedScripts{ };

  PapyrusScript* loadScript(const std::string& name);
};

}}
