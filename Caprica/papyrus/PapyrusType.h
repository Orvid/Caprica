#pragma once

#include <cstring>
#include <memory>
#include <string>

#include <common/CapricaError.h>
#include <common/CapricaFileLocation.h>

#include <pex/PexFile.h>
#include <pex/PexString.h>

namespace caprica { namespace papyrus {

struct PapyrusObject;
struct PapyrusStruct;

struct PapyrusType final
{
  enum class Kind
  {
    None,
    Bool,
    Float,
    Int,
    String,
    Var,

    Array,
    Unresolved,
    ResolvedStruct,
    ResolvedObject,
  };

  // This is intended purely for initializing types that will be
  // assigned fully later in the control flow.
  struct Default final { };

  Kind type{ Kind::None };
  std::string name{ };
  CapricaFileLocation location;
  union
  {
    const PapyrusStruct* resolvedStruct{ nullptr };
    const PapyrusObject* resolvedObject;
  };

  PapyrusType() = delete;
  PapyrusType(const Default& other) : type(Kind::Unresolved), location(CapricaFileLocation{ "", 0, 0 }) { }
  PapyrusType(const PapyrusType& other) = default;
  PapyrusType(PapyrusType&& other) = default;
  PapyrusType& operator =(const PapyrusType&) = default;
  PapyrusType& operator =(PapyrusType&&) = default;
  ~PapyrusType() = default;

  static PapyrusType Unresolved(const CapricaFileLocation& loc, const std::string& nm) {
    auto pt = PapyrusType(Kind::Unresolved, loc);
    pt.name = nm;
    return pt;
  }

  static PapyrusType Array(const CapricaFileLocation& loc, std::shared_ptr<PapyrusType> tp) {
    auto pt = PapyrusType(Kind::Array, loc);
    pt.arrayElementType = tp;
    return pt;
  }

  static PapyrusType None(const CapricaFileLocation& loc) { return PapyrusType(Kind::None, loc); }
  static PapyrusType Bool(const CapricaFileLocation& loc) { return PapyrusType(Kind::Bool, loc); }
  static PapyrusType Float(const CapricaFileLocation& loc) { return PapyrusType(Kind::Float, loc); }
  static PapyrusType Int(const CapricaFileLocation& loc) { return PapyrusType(Kind::Int, loc); }
  static PapyrusType String(const CapricaFileLocation& loc) { return PapyrusType(Kind::String, loc); }
  static PapyrusType Var(const CapricaFileLocation& loc) { return PapyrusType(Kind::Var, loc); }
  static PapyrusType ResolvedObject(const CapricaFileLocation& loc, const PapyrusObject* obj) {
    auto pt = PapyrusType(Kind::ResolvedObject, loc);
    pt.resolvedObject = obj;
    return pt;
  }

  pex::PexString buildPex(pex::PexFile* file) const {
    return file->getString(getTypeString());
  }

  PapyrusType getElementType() const {
    assert(type == Kind::Array);
    return *arrayElementType;
  }

  std::string prettyString() const;

  bool operator ==(const PapyrusType& other) const {
    return !(*this != other);
  }

  bool operator !=(const PapyrusType& other) const {
    if (type == other.type) {
      switch (type) {
        case Kind::None:
        case Kind::Bool:
        case Kind::Float:
        case Kind::Int:
        case Kind::String:
        case Kind::Var:
          return false;
        case Kind::Array:
          return *arrayElementType != *other.arrayElementType;
        case Kind::Unresolved:
          return _stricmp(name.c_str(), other.name.c_str()) != 0;
        case Kind::ResolvedStruct:
          return resolvedStruct != other.resolvedStruct;
        case Kind::ResolvedObject:
          return resolvedObject != other.resolvedObject;
      }
      CapricaError::logicalFatal("Unknown PapyrusTypeKind while comparing!");
    }
    return true;
  }

private:
  std::shared_ptr<PapyrusType> arrayElementType{ nullptr };

  PapyrusType(Kind k, const CapricaFileLocation& loc) : type(k), location(loc) { }

  std::string getTypeString() const;
};

}}
