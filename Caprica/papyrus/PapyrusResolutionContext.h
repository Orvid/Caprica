#pragma once

#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/range/adaptor/reversed.hpp>

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

  void addImport(std::string import);
  PapyrusType resolveType(PapyrusType tp);

  void ensureCastable(PapyrusType src, PapyrusType dest) {

  }

  [[noreturn]]
  void fatalError(const std::string& msg) const {
    // TODO: Expand on this, making sure to write things like the
    // line number to stderr before dying.
    throw std::runtime_error(msg);
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
      throw std::runtime_error("Unresolved identifier '" + ident.name + "'!");
    return id;
  }

  PapyrusIdentifier tryResolveIdentifier(const PapyrusIdentifier& ident) const {
    if (ident.type != PapyrusIdentifierType::Unresolved) {
      return ident;
    }

    for (auto& stack : boost::adaptors::reverse(identifierStack)) {
      auto f = stack.find(ident.name);
      if (f != stack.end()) {
        return f->second;
      }
    }

    return ident;
  }

  PapyrusIdentifier resolveMemberIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const;
  PapyrusIdentifier resolveFunctionIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const;

  ~PapyrusResolutionContext();
private:
  std::vector<std::map<std::string, PapyrusIdentifier, parser::CaselessStringComparer>> identifierStack{ };
  std::vector<PapyrusScript*> importedScripts{ };
  std::map<std::string, PapyrusScript*, parser::CaselessStringComparer> loadedScripts{ };

  PapyrusScript* loadScript(std::string name);
};

}}
