#pragma once

#include <cstring>
#include <memory>
#include <string>

#include <common/CapricaFileLocation.h>
#include <common/CapricaReportingContext.h>
#include <common/identifier_ref.h>
#include <common/LargelyBufferedString.h>

#include <pex/PexFile.h>
#include <pex/PexString.h>

namespace caprica { namespace papyrus {

struct PapyrusObject;
struct PapyrusStruct;

struct PapyrusType final {
  enum class PoisonKind {
    None = 0b00000000,
    Beta = 0b00000001,
    Debug = 0b00000010,
  };

  enum class Kind {
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

  Kind type { Kind::None };
  PoisonKind poisonState { PoisonKind::None };
  CapricaFileLocation location {};
  union ResolvedData {
    const PapyrusStruct* struc { nullptr };
    const PapyrusObject* obj;
    const PapyrusType* arrayElementType;
  } resolved;

  PapyrusType() = delete;
  PapyrusType(const Default&) : type(Kind::Unresolved) { }
  PapyrusType(const PapyrusType& other) = default;
  PapyrusType(PapyrusType&& other) = default;
  PapyrusType& operator=(const PapyrusType&) = default;
  PapyrusType& operator=(PapyrusType&&) = default;
  ~PapyrusType() = default;

  static PapyrusType Unresolved(CapricaFileLocation loc, const std::string& nm) = delete;
  static PapyrusType Unresolved(CapricaFileLocation loc, std::string&& nm) = delete;
  static PapyrusType Unresolved(CapricaFileLocation loc, const char* nm) {
    auto pt = PapyrusType(Kind::Unresolved, loc);
    pt.name = nm;
    return pt;
  }
  static PapyrusType Unresolved(CapricaFileLocation loc, const identifier_ref& nm) {
    auto pt = PapyrusType(Kind::Unresolved, loc);
    pt.name = nm;
    return pt;
  }

  static PapyrusType Array(CapricaFileLocation loc, PapyrusType* tp) {
    auto pt = PapyrusType(Kind::Array, loc);
    pt.resolved.arrayElementType = tp;
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
    pt.resolved.obj = obj;
    return pt;
  }
  static PapyrusType ResolvedStruct(CapricaFileLocation loc, const PapyrusStruct* struc) {
    auto pt = PapyrusType(Kind::ResolvedStruct, loc);
    pt.resolved.struc = struc;
    return pt;
  }

  pex::PexString buildPex(pex::PexFile* file) const {
    LargelyBufferedString buf;
    getTypeStringAsRef(buf);
    return file->getString(buf.string_view());
  }

  const PapyrusType& getElementType() const& {
    assert(type == Kind::Array);
    return *resolved.arrayElementType;
  }

  std::string prettyString() const;

  void poison(PoisonKind kind);
  bool isPoisoned(PoisonKind kind) const;

  bool operator!=(const PapyrusType& other) const;
  bool operator==(const PapyrusType& other) const { return !(*this != other); }

private:
  friend struct PapyrusResolutionContext;

  identifier_ref name {};

  PapyrusType(Kind k, CapricaFileLocation loc) : type(k), location(loc) { }

  LargelyBufferedString& getTypeStringAsRef(LargelyBufferedString& buf) const;
};

inline auto operator~(PapyrusType::PoisonKind a) {
  return (decltype(a))(~(std::underlying_type<decltype(a)>::type)a);
}
inline auto operator&(PapyrusType::PoisonKind a, PapyrusType::PoisonKind b) {
  return (decltype(a))((std::underlying_type<decltype(a)>::type)a & (std::underlying_type<decltype(b)>::type)b);
}
inline auto operator|(PapyrusType::PoisonKind a, PapyrusType::PoisonKind b) {
  return (decltype(a))((std::underlying_type<decltype(a)>::type)a | (std::underlying_type<decltype(b)>::type)b);
}
inline auto& operator&=(PapyrusType::PoisonKind& a, PapyrusType::PoisonKind b) {
  return a = a & b;
}
inline auto& operator|=(PapyrusType::PoisonKind& a, PapyrusType::PoisonKind b) {
  return a = a | b;
}

}}
