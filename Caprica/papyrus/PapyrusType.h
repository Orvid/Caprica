#pragma once

#include <cstring>
#include <memory>
#include <string>

#include <common/CapricaFileLocation.h>
#include <common/CapricaReportingContext.h>

#include <pex/PexFile.h>
#include <pex/PexString.h>

namespace caprica { namespace papyrus {

struct PapyrusObject;
struct PapyrusStruct;

struct PapyrusType final
{
  enum class PoisonKind
  {
    None  = 0b00000000,
    Beta  = 0b00000001,
    Debug = 0b00000010,
  };

  enum class Kind
  {
    None,
    Bool,
    Float,
    Int,
    String,
    Var,

    CustomEventName,
    ScriptEventName,

    Array,
    Unresolved,
    ResolvedStruct,
    ResolvedObject,
  };

  // This is intended purely for initializing types that will be
  // assigned fully later in the control flow.
  struct Default final { };

  Kind type{ Kind::None };
  CapricaFileLocation location{ };
  union
  {
    const PapyrusStruct* resolvedStruct{ nullptr };
    const PapyrusObject* resolvedObject;
  };

  PapyrusType() = delete;
  PapyrusType(const Default& other) : type(Kind::Unresolved) { }
  PapyrusType(const PapyrusType& other) = default;
  PapyrusType(PapyrusType&& other) = default;
  PapyrusType& operator =(const PapyrusType&) = default;
  PapyrusType& operator =(PapyrusType&&) = default;
  ~PapyrusType() = default;

  static PapyrusType Unresolved(CapricaFileLocation loc, const std::string& nm) {
    auto pt = PapyrusType(Kind::Unresolved, loc);
    pt.name = nm;
    return pt;
  }
  static PapyrusType Unresolved(CapricaFileLocation loc, std::string&& nm) {
    auto pt = PapyrusType(Kind::Unresolved, loc);
    pt.name = std::move(nm);
    return pt;
  }

  static PapyrusType Array(CapricaFileLocation loc, std::shared_ptr<PapyrusType> tp) {
    auto pt = PapyrusType(Kind::Array, loc);
    pt.arrayElementType = tp;
    return pt;
  }

  static PapyrusType None(CapricaFileLocation loc) { return PapyrusType(Kind::None, loc); }
  static PapyrusType PoisonedNone(CapricaFileLocation loc, const PapyrusType& poisonSource) {
    auto pt = PapyrusType(Kind::None, loc);
    pt.poisonState = poisonSource.poisonState;
    return pt;
  }
  static PapyrusType Bool(CapricaFileLocation loc) { return PapyrusType(Kind::Bool, loc); }
  static PapyrusType Float(CapricaFileLocation loc) { return PapyrusType(Kind::Float, loc); }
  static PapyrusType Int(CapricaFileLocation loc) { return PapyrusType(Kind::Int, loc); }
  static PapyrusType String(CapricaFileLocation loc) { return PapyrusType(Kind::String, loc); }
  static PapyrusType Var(CapricaFileLocation loc) { return PapyrusType(Kind::Var, loc); }
  static PapyrusType ResolvedObject(CapricaFileLocation loc, const PapyrusObject* obj) {
    auto pt = PapyrusType(Kind::ResolvedObject, loc);
    pt.resolvedObject = obj;
    return pt;
  }
  static PapyrusType ResolvedStruct(CapricaFileLocation loc, const PapyrusStruct* struc) {
    auto pt = PapyrusType(Kind::ResolvedStruct, loc);
    pt.resolvedStruct = struc;
    return pt;
  }

  pex::PexString buildPex(pex::PexFile* file) const {
    return file->getString(getTypeString());
  }

  const PapyrusType& getElementType() const& {
    assert(type == Kind::Array);
    return *arrayElementType;
  }

  std::string prettyString() const;

  void poison(PoisonKind kind);
  bool isPoisoned(PoisonKind kind) const;

  bool operator !=(const PapyrusType& other) const;
  bool operator ==(const PapyrusType& other) const {
    return !(*this != other);
  }

private:
  friend struct PapyrusResolutionContext;

  std::string name{ };
  PoisonKind poisonState{ PoisonKind::None };
  std::shared_ptr<PapyrusType> arrayElementType{ nullptr };

  PapyrusType(Kind k, CapricaFileLocation loc) : type(k), location(loc) { }

  std::string getTypeString() const;
};

inline auto operator ~(PapyrusType::PoisonKind a) {
  return (decltype(a))(~(std::underlying_type<decltype(a)>::type)a);
}
inline auto operator &(PapyrusType::PoisonKind a, PapyrusType::PoisonKind b) {
  return (decltype(a))((std::underlying_type<decltype(a)>::type)a & (std::underlying_type<decltype(b)>::type)b);
}
inline auto operator |(PapyrusType::PoisonKind a, PapyrusType::PoisonKind b) {
  return (decltype(a))((std::underlying_type<decltype(a)>::type)a | (std::underlying_type<decltype(b)>::type)b);
}
inline auto& operator &=(PapyrusType::PoisonKind& a, PapyrusType::PoisonKind b) {
  return a = a & b;
}
inline auto& operator |=(PapyrusType::PoisonKind& a, PapyrusType::PoisonKind b) {
  return a = a | b;
}

}}
