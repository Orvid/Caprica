#pragma once

#include <string>

#include <common/CapricaFileLocation.h>
#include <common/CapricaReportingContext.h>

#include <papyrus/PapyrusType.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus {

struct PapyrusFunction;
struct PapyrusFunctionParameter;
struct PapyrusProperty;
struct PapyrusResolutionContext;
struct PapyrusStructMember;
struct PapyrusVariable;

namespace expressions { struct PapyrusMemberAccessExpression; }
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

  static PapyrusIdentifier Unresolved(CapricaFileLocation loc, const std::string& nm) {
    auto id = PapyrusIdentifier(PapyrusIdentifierType::Unresolved, loc);
    id.name = nm;
    return id;
  }
  static PapyrusIdentifier Unresolved(CapricaFileLocation loc, std::string&& nm) {
    auto id = PapyrusIdentifier(PapyrusIdentifierType::Unresolved, loc);
    id.name = std::move(nm);
    return id;
  }
  static PapyrusIdentifier Property(CapricaFileLocation loc, PapyrusProperty* p);
  static PapyrusIdentifier Variable(CapricaFileLocation loc, PapyrusVariable* v);
  static PapyrusIdentifier FunctionParameter(CapricaFileLocation loc, PapyrusFunctionParameter* p);
  static PapyrusIdentifier DeclStatement(CapricaFileLocation loc, statements::PapyrusDeclareStatement* s);
  static PapyrusIdentifier StructMember(CapricaFileLocation loc, PapyrusStructMember* m);
  static PapyrusIdentifier Function(CapricaFileLocation loc, PapyrusFunction* f);
  static PapyrusIdentifier ArrayFunction(CapricaFileLocation loc, PapyrusBuiltinArrayFunctionKind fk, const PapyrusType& elemType);

  pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue::Identifier base) const;
  void generateStore(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue::Identifier base, pex::PexValue val) const;
  void ensureAssignable(CapricaReportingContext& repCtx) const;
  void markRead();
  void markWritten();
  PapyrusType resultType() const;

private:
  friend expressions::PapyrusMemberAccessExpression;
  friend PapyrusResolutionContext;

  std::string name{ };

  PapyrusIdentifier(PapyrusIdentifierType k, CapricaFileLocation loc) : type(k), location(loc) { }
};

}}
