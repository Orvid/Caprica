#pragma once

#include <string>

namespace caprica { namespace papyrus { struct PapyrusResolutionContext; } }

#include <papyrus/PapyrusType.h>

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

  void addImport(std::string import) {

  }

  PapyrusType resolveType(PapyrusType tp) {
    return tp;
  }

  void ensureCastable(PapyrusType src, PapyrusType dest) {

  }

  [[noreturn]]
  void fatalError(const std::string& msg) {
    // TODO: Expand on this, making sure to write things like the
    // line number to stderr before dying.
    throw std::runtime_error(msg);
  }
};

}}
