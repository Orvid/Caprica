#pragma once

#include <string>

#include <CapricaError.h>

#include <papyrus/PapyrusType.h>
#include <papyrus/parser/PapyrusFileLocation.h>

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
  parser::PapyrusFileLocation location;
  PapyrusType arrayFuncElementType{ PapyrusType::None({ "", 0, 0 }) };
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

  struct Unresolved final
  {
    const parser::PapyrusFileLocation location;
    const std::string name;

    Unresolved(const parser::PapyrusFileLocation& loc, const std::string& nm) : location(loc), name(nm) { }
    ~Unresolved() = default;
  };
  struct Property final
  {
    const parser::PapyrusFileLocation location;
    PapyrusProperty* prop;

    Property(const parser::PapyrusFileLocation& loc, PapyrusProperty* p) : location(loc), prop(p) { }
    ~Property() = default;
  };
  struct Variable final
  {
    const parser::PapyrusFileLocation location;
    PapyrusVariable* var;

    Variable(const parser::PapyrusFileLocation& loc, PapyrusVariable* v) : location(loc), var(v) { }
    ~Variable() = default;
  };
  struct FunctionParameter final
  {
    const parser::PapyrusFileLocation location;
    PapyrusFunctionParameter* param;

    FunctionParameter(const parser::PapyrusFileLocation& loc, PapyrusFunctionParameter* p) : location(loc), param(p) { }
    ~FunctionParameter() = default;
  };
  struct DeclStatement final
  {
    const parser::PapyrusFileLocation location;
    statements::PapyrusDeclareStatement* declStatement;

    DeclStatement(const parser::PapyrusFileLocation& loc, statements::PapyrusDeclareStatement* s) : location(loc), declStatement(s) { }
    ~DeclStatement() = default;
  };
  struct StructMember final
  {
    const parser::PapyrusFileLocation location;
    PapyrusStructMember* member;

    StructMember(const parser::PapyrusFileLocation& loc, PapyrusStructMember* m) : location(loc), member(m) { }
    ~StructMember() = default;
  };
  struct Function final
  {
    const parser::PapyrusFileLocation location;
    PapyrusFunction* function;

    Function(const parser::PapyrusFileLocation& loc, PapyrusFunction* f) : location(loc), function(f) { }
    ~Function() = default;
  };
  struct ArrayFunction final
  {
    const parser::PapyrusFileLocation location;
    PapyrusBuiltinArrayFunctionKind arrayFuncKind;
    PapyrusType arrayFuncElementType;

    ArrayFunction(const parser::PapyrusFileLocation& loc, PapyrusBuiltinArrayFunctionKind fk, const PapyrusType& elemType) : location(loc), arrayFuncKind(fk), arrayFuncElementType(elemType) { }
    ~ArrayFunction() = default;
  };

  PapyrusIdentifier(const Unresolved& other) : type(PapyrusIdentifierType::Unresolved), location(other.location), name(other.name) { }
  PapyrusIdentifier(const Property& other);
  PapyrusIdentifier(const Variable& other);
  PapyrusIdentifier(const FunctionParameter& other);
  PapyrusIdentifier(const DeclStatement& other);
  PapyrusIdentifier(const StructMember& other);
  PapyrusIdentifier(const Function& other);
  PapyrusIdentifier(const ArrayFunction& other);
  PapyrusIdentifier(const PapyrusIdentifier& other) = default;
  ~PapyrusIdentifier() = default;

  pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue::Identifier base) const;
  void PapyrusIdentifier::generateStore(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue::Identifier base, pex::PexValue val) const;
  PapyrusType resultType() const;
};

}}
