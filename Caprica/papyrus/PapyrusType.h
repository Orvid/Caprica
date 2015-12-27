#pragma once

#include <string>

#include <pex/PexFile.h>
#include <pex/PexString.h>

namespace caprica { namespace papyrus {

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

    Unresolved,
  };

  struct Unresolved
  {
    std::string name;

    Unresolved(std::string nm) : name(nm) { }
    ~Unresolved() = default;
  };
  struct Array
  {
    std::string name;

    Array(std::string nm) : name(nm) { }
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
  bool isArray{ false };

  PapyrusType() = default;

  PapyrusType(const Unresolved& other) : type(Kind::Unresolved), name(other.name) { }
  PapyrusType(const Array& other) : type(Kind::Unresolved), name(other.name), isArray(true) { }

  PapyrusType(const None& other) : type(Kind::None) { }
  PapyrusType(const Bool& other) : type(Kind::Bool) { }
  PapyrusType(const Float& other) : type(Kind::Float) { }
  PapyrusType(const Int& other) : type(Kind::Int) { }
  PapyrusType(const String& other) : type(Kind::String) { }
  PapyrusType(const Var& other) : type(Kind::Var) { }
  PapyrusType(const PapyrusType& other) = default;

  pex::PexString buildPex(pex::PexFile* file) const {
    std::string str;
    switch (type) {
      case Kind::None:
        str = "None";
        break;
      case Kind::Bool:
        str = "Bool";
        break;
      case Kind::Float:
        str = "Float";
        break;
      case Kind::Int:
        str = "Int";
        break;
      case Kind::String:
        str = "String";
        break;
      case Kind::Var:
        str = "Var";
        break;
      case Kind::Unresolved:
        str = name;
        break;
      default:
        throw std::runtime_error("Unknown PapyrusTypeKind!");
    }
    if (isArray)
      str += "[]";
    return file->getString(str);
  }

  bool operator ==(const PapyrusType& other) const {
    return !(*this != other);
  }

  bool operator !=(const PapyrusType& other) const {
    if (type == other.type && isArray == other.isArray) {
      switch (type) {
        case Kind::None:
        case Kind::Bool:
        case Kind::Float:
        case Kind::Int:
        case Kind::String:
        case Kind::Var:
          return false;
        case Kind::Unresolved:
          return _stricmp(name.c_str(), other.name.c_str()) != 0;
        default:
          throw std::runtime_error("Unknown PapyrusTypeKind while comparing!");
      }
    }
    return true;
  }
};

}}
