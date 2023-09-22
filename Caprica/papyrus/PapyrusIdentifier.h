#pragma once

#include <string>

#include <common/CapricaFileLocation.h>
#include <common/CapricaReportingContext.h>
#include <common/identifier_ref.h>

#include <papyrus/PapyrusType.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus {

struct PapyrusFunction;
struct PapyrusFunctionParameter;
struct PapyrusGuard;
struct PapyrusProperty;
struct PapyrusResolutionContext;
struct PapyrusStructMember;
struct PapyrusVariable;

namespace expressions {
struct PapyrusMemberAccessExpression;
}
namespace statements {
struct PapyrusDeclareStatement;
}

enum class PapyrusIdentifierType : uint16_t {
  Unresolved,

  Property,
  Guard,
  Variable,
  Parameter,
  DeclareStatement,
  StructMember,
  Function,
  BuiltinArrayFunction,
  BuiltinStateField,
};

enum class PapyrusBuiltinArrayFunctionKind : uint16_t {
  Unknown = 0,

  Find,
  RFind,
  FindStruct,
  RFindStruct,

  Add,
  Clear,
  Insert,
  Remove,
  RemoveLast,
  // Fallout 76, Starfield
  GetMatchingStructs,
  ArrayFunctionMax,
  SkyrimArrayFunctionMax = RFind,
  Fallout4ArrayFunctionMax = RemoveLast,
  Fallout76ArrayFunctionMax = GetMatchingStructs,
  StarfieldArrayFunctionMax = GetMatchingStructs,
  // Skyrim
};

constexpr bool isArrayFunctionInGame(PapyrusBuiltinArrayFunctionKind fk, GameID game) {
  switch (game) {
    case GameID::Skyrim:
      return fk <= PapyrusBuiltinArrayFunctionKind::SkyrimArrayFunctionMax;
    case GameID::Fallout4:
      return fk <= PapyrusBuiltinArrayFunctionKind::Fallout4ArrayFunctionMax;
    case GameID::Fallout76:
      return fk <= PapyrusBuiltinArrayFunctionKind::Fallout76ArrayFunctionMax;
    case GameID::Starfield:
      return fk <= PapyrusBuiltinArrayFunctionKind::StarfieldArrayFunctionMax;
    default:
      return false;
  }
}

struct PapyrusIdentifier final {
  PapyrusIdentifierType type { PapyrusIdentifierType::Unresolved };
  PapyrusBuiltinArrayFunctionKind arrayFuncKind { PapyrusBuiltinArrayFunctionKind::Unknown };
  CapricaFileLocation location;
  union ResolvedTargets {
    identifier_ref name {};
    const PapyrusProperty* prop;
    const PapyrusGuard* guard;
    const PapyrusVariable* var;
    const PapyrusFunctionParameter* param;
    const statements::PapyrusDeclareStatement* declStatement;
    const PapyrusStructMember* structMember;
    const PapyrusFunction* func;
    PapyrusType* arrayFuncElementType;

    ResolvedTargets() : name() { }
  } res;

  PapyrusIdentifier() = delete;
  PapyrusIdentifier(const PapyrusIdentifier&) = default;
  PapyrusIdentifier(PapyrusIdentifier&&) = default;
  PapyrusIdentifier& operator=(const PapyrusIdentifier&) = default;
  PapyrusIdentifier& operator=(PapyrusIdentifier&&) = default;
  ~PapyrusIdentifier() = default;

  static PapyrusIdentifier Unresolved(CapricaFileLocation loc, const std::string& nm) = delete;
  static PapyrusIdentifier Unresolved(CapricaFileLocation loc, std::string&& nm) = delete;
  static PapyrusIdentifier Unresolved(CapricaFileLocation loc, const char* nm) {
    auto id = PapyrusIdentifier(PapyrusIdentifierType::Unresolved, loc);
    id.res.name = nm;
    return id;
  }
  static PapyrusIdentifier Unresolved(CapricaFileLocation loc, const identifier_ref& nm) {
    auto id = PapyrusIdentifier(PapyrusIdentifierType::Unresolved, loc);
    id.res.name = nm;
    return id;
  }
  static PapyrusIdentifier Property(CapricaFileLocation loc, const PapyrusProperty* p);
  static PapyrusIdentifier Guard(CapricaFileLocation loc, const PapyrusGuard* g);
  static PapyrusIdentifier Variable(CapricaFileLocation loc, const PapyrusVariable* v);
  static PapyrusIdentifier FunctionParameter(CapricaFileLocation loc, const PapyrusFunctionParameter* p);
  static PapyrusIdentifier DeclStatement(CapricaFileLocation loc, const statements::PapyrusDeclareStatement* s);
  static PapyrusIdentifier StructMember(CapricaFileLocation loc, const PapyrusStructMember* m);
  static PapyrusIdentifier Function(CapricaFileLocation loc, const PapyrusFunction* f);
  static PapyrusIdentifier
  ArrayFunction(CapricaFileLocation loc, PapyrusBuiltinArrayFunctionKind fk, PapyrusType* elemType);

  static const char* prettyTypeString(PapyrusIdentifierType t) {
    switch (t) {
      case PapyrusIdentifierType::Unresolved:
        return "Unresolved";
      case PapyrusIdentifierType::Property:
        return "Property";
      case PapyrusIdentifierType::Guard:
        return "Guard";
      case PapyrusIdentifierType::Variable:
        return "Variable";
      case PapyrusIdentifierType::Parameter:
        return "Parameter";
      case PapyrusIdentifierType::DeclareStatement:
        return "Local Variable";
      case PapyrusIdentifierType::StructMember:
        return "Struct Member";
      case PapyrusIdentifierType::Function:
        return "Function";
      case PapyrusIdentifierType::BuiltinArrayFunction:
        return "Built-in Array Function";
      case PapyrusIdentifierType::BuiltinStateField:
        return "Built-in State Field";
    }
    return "";
  }

  pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue::Identifier base) const;
  void generateStore(pex::PexFile* file,
                     pex::PexFunctionBuilder& bldr,
                     pex::PexValue::Identifier base,
                     pex::PexValue val) const;
  void ensureAssignable(CapricaReportingContext& repCtx) const;
  void markRead();
  void markWritten();
  PapyrusType resultType() const;

private:
  friend expressions::PapyrusMemberAccessExpression;
  friend PapyrusResolutionContext;

  PapyrusIdentifier(PapyrusIdentifierType k, CapricaFileLocation loc) : type(k), location(loc) { }
};

}}
