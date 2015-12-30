#pragma once

#include <cstring>
#include <memory>
#include <string>

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

  struct Unresolved
  {
    std::string name;

    Unresolved(std::string nm) : name(nm) { }
    ~Unresolved() = default;
  };
  struct Array
  {
    PapyrusType* type;

    Array(PapyrusType* tp) : type(tp) { }
    ~Array() = default;
  };
  struct None { };
  struct Bool { };
  struct Float { };
  struct Int { };
  struct String { };
  struct Var { };

  Kind type{ Kind::None };
  std::string name{ "" };
  std::shared_ptr<PapyrusType> arrayElementType{ nullptr };
  union
  {
    const PapyrusStruct* resolvedStruct{ nullptr };
    const PapyrusObject* resolvedObject;
  };

  PapyrusType() = default;

  PapyrusType(const Unresolved& other) : type(Kind::Unresolved), name(other.name) { }
  PapyrusType(const Array& other) : type(Kind::Array), arrayElementType(other.type) { }

  PapyrusType(const None& other) : type(Kind::None) { }
  PapyrusType(const Bool& other) : type(Kind::Bool) { }
  PapyrusType(const Float& other) : type(Kind::Float) { }
  PapyrusType(const Int& other) : type(Kind::Int) { }
  PapyrusType(const String& other) : type(Kind::String) { }
  PapyrusType(const Var& other) : type(Kind::Var) { }
  PapyrusType(const PapyrusType& other) = default;

  pex::PexString buildPex(pex::PexFile* file) const {
    return file->getString(getTypeString());
  }

  PapyrusType getElementType() const {
    assert(type == Kind::Array);
    return *arrayElementType;
  }

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
        default:
          throw std::runtime_error("Unknown PapyrusTypeKind while comparing!");
      }
    }
    return true;
  }

private:
  std::string getTypeString() const;
};

}}
