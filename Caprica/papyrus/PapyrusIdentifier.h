#pragma once

#include <string>

#include <common/CapricaError.h>

#include <common/CapricaFileLocation.h>
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
  BuiltinStateField,
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
  std::string name{ };
  CapricaFileLocation location;
  std::shared_ptr<PapyrusType> arrayFuncElementType{ nullptr };
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

  PapyrusIdentifier() = delete;
  PapyrusIdentifier(const PapyrusIdentifier& other) = default;
  PapyrusIdentifier(PapyrusIdentifier&& other) = default;
  PapyrusIdentifier& operator =(const PapyrusIdentifier&) = default;
  PapyrusIdentifier& operator =(PapyrusIdentifier&&) = default;
  ~PapyrusIdentifier() = default;

  static PapyrusIdentifier Unresolved(const CapricaFileLocation& loc, const std::string& nm) {
    auto id = PapyrusIdentifier(PapyrusIdentifierType::Unresolved, loc);
    id.name = nm;
    return id;
  }
  static PapyrusIdentifier Property(const CapricaFileLocation& loc, PapyrusProperty* p);
  static PapyrusIdentifier Variable(const CapricaFileLocation& loc, PapyrusVariable* v);
  static PapyrusIdentifier FunctionParameter(const CapricaFileLocation& loc, PapyrusFunctionParameter* p);
  static PapyrusIdentifier DeclStatement(const CapricaFileLocation& loc, statements::PapyrusDeclareStatement* s);
  static PapyrusIdentifier StructMember(const CapricaFileLocation& loc, PapyrusStructMember* m);
  static PapyrusIdentifier Function(const CapricaFileLocation& loc, PapyrusFunction* f);
  static PapyrusIdentifier ArrayFunction(const CapricaFileLocation& loc, PapyrusBuiltinArrayFunctionKind fk, const PapyrusType& elemType);

  pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue::Identifier base) const;
  void generateStore(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue::Identifier base, pex::PexValue val) const;
  void ensureAssignable() const;
  void markRead();
  void markWritten();
  PapyrusType resultType() const;

private:
  PapyrusIdentifier(PapyrusIdentifierType k, const CapricaFileLocation& loc) : type(k), location(loc) { }
};

}}
