#pragma once

#include <string>

#include <pex/PexFile.h>
#include <pex/PexString.h>

namespace caprica { namespace papyrus {

struct PapyrusType final
{
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

  std::string name{ "" };

  PapyrusType() = default;

  PapyrusType(const Unresolved& other) : name(other.name) { }
  PapyrusType(const Array& other) : name(other.name + "[]") { }

  PapyrusType(const None& other) : name("None") { }
  PapyrusType(const Bool& other) : name("Bool") { }
  PapyrusType(const Float& other) : name("Float") { }
  PapyrusType(const Int& other) : name("Int") { }
  PapyrusType(const String& other) : name("String") { }
  PapyrusType(const Var& other) : name("Var") { }
  PapyrusType(const PapyrusType& other) = default;
  // TODO: Remove this overload; this is only for testing.
  PapyrusType(std::string nm) : name(nm) {  }

  pex::PexString buildPex(pex::PexFile* file) const {
    return file->getString(name);
  }

  bool operator !=(const PapyrusType& other) const {
    return _stricmp(name.c_str(), other.name.c_str()) != 0;
  }
};

}}
