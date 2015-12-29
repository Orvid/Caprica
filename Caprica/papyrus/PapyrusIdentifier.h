#pragma once

#include <string>

#include <papyrus/PapyrusType.h>

namespace caprica { namespace papyrus {

struct PapyrusFunctionParameter;
struct PapyrusProperty;
struct PapyrusVariable;

namespace statements { struct PapyrusDeclareStatement; }

enum class PapyrusIdentifierType
{
  Unresolved,

  Property,
  Variable,
  Parameter,
  DeclareStatement,
};

struct PapyrusIdentifier final
{
  PapyrusIdentifierType type{ PapyrusIdentifierType::Unresolved };
  std::string name{ "" };
  union
  {
    PapyrusProperty* prop{ nullptr };
    PapyrusVariable* var;
    PapyrusFunctionParameter* param;
    statements::PapyrusDeclareStatement* declStatement;
  };

  PapyrusIdentifier() = default;
  ~PapyrusIdentifier() = default;

  PapyrusType resultType() const;
};

}}
