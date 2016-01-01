#pragma once

#include <string>

#include <papyrus/PapyrusType.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus {

struct PapyrusFunction;
struct PapyrusFunctionParameter;
struct PapyrusProperty;
struct PapyrusStructMember;
struct PapyrusVariable;

namespace statements { struct PapyrusDeclareStatement; }

enum class PapyrusIdentifierType
{
  Unresolved,

  Property,
  Variable,
  Parameter,
  DeclareStatement,
  StructMember,
  Function,
  BuiltinArrayFunction,
};

enum class PapyrusBuiltinArrayFunctionKind
{
  Unknown = 0,

  Find,
  FindStruct,
  RFind,
  RFindStruct,

  Add,
  Clear,
  Insert,
  Remove,
  RemoveLast,
};

struct PapyrusIdentifier final
{
  PapyrusIdentifierType type{ PapyrusIdentifierType::Unresolved };
  std::string name{ "" };
  PapyrusType arrayFuncElementType{ };
  union
  {
    PapyrusProperty* prop{ nullptr };
    PapyrusVariable* var;
    PapyrusFunctionParameter* param;
    statements::PapyrusDeclareStatement* declStatement;
    PapyrusStructMember* structMember;
    PapyrusFunction* func;
    PapyrusBuiltinArrayFunctionKind arrayFuncKind;
  };

  PapyrusIdentifier() = default;
  ~PapyrusIdentifier() = default;

  pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue::Identifier base) const;
  void PapyrusIdentifier::generateStore(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue::Identifier base, pex::PexValue val) const;

  PapyrusType resultType() const;
};

}}
