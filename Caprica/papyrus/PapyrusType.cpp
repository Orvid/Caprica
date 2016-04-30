#include <papyrus/PapyrusType.h>

#include <boost/algorithm/string/case_conv.hpp>

#include <papyrus/PapyrusObject.h>
#include <papyrus/PapyrusStruct.h>

namespace caprica { namespace papyrus {

void PapyrusType::poison(PapyrusType::PoisonKind kind) {
  this->poisonState |= kind;
}

bool PapyrusType::isPoisoned(PoisonKind kind) const {
  return (this->poisonState & kind) == kind;
}

std::string PapyrusType::prettyString() const {
  switch (type) {
    case Kind::None:
      return "None";
    case Kind::Bool:
      return "bool";
    case Kind::Float:
      return "float";
    case Kind::Int:
      return "int";
    case Kind::String:
      return "string";
    case Kind::Var:
      return "var";
    case Kind::CustomEventName:
      return "CustomEventName";
    case Kind::ScriptEventName:
      return "ScriptEventName";
    case Kind::Array:
      return arrayElementType->prettyString() + "[]";
    case Kind::Unresolved:
      return "unresolved(" + name + ")";
    case Kind::ResolvedObject:
      return resolvedObject->name;
    case Kind::ResolvedStruct:
      return "struct " + resolvedStruct->parentObject->name + "." + resolvedStruct->name;
  }
  CapricaReportingContext::logicalFatal("Unknown PapyrusTypeKind!");
}

std::string PapyrusType::getTypeString() const {
  switch (type) {
    case Kind::None:
      return "None";
    case Kind::Bool:
      return "Bool";
    case Kind::Float:
      return "Float";
    case Kind::Int:
      return "Int";
    case Kind::String:
    case Kind::CustomEventName: // These are both really strings.
    case Kind::ScriptEventName:
      return "String";
    case Kind::Var:
      return "Var";
    case Kind::Array:
      return arrayElementType->getTypeString() + "[]";
    case Kind::Unresolved:
      return name;
    case Kind::ResolvedObject:
    {
      auto lowerType = resolvedObject->name;
      boost::algorithm::to_lower(lowerType);
      return lowerType;
    }
    case Kind::ResolvedStruct:
    {
      auto name = resolvedStruct->parentObject->name + "#" + resolvedStruct->name;
      boost::algorithm::to_lower(name);
      return name;
    }
  }
  CapricaReportingContext::logicalFatal("Unknown PapyrusTypeKind!");
}

bool PapyrusType::operator !=(const PapyrusType& other) const {
  if (type == other.type) {
    switch (type) {
      case Kind::None:
      case Kind::Bool:
      case Kind::Float:
      case Kind::Int:
      case Kind::String:
      case Kind::Var:
      case Kind::CustomEventName:
      case Kind::ScriptEventName:
        return false;
      case Kind::Array:
        return *arrayElementType != *other.arrayElementType;
      case Kind::Unresolved:
        return _stricmp(name.c_str(), other.name.c_str()) != 0;
      case Kind::ResolvedStruct:
        if (resolvedStruct == other.resolvedStruct)
          return false;
        if (!_stricmp(resolvedStruct->name.c_str(), other.resolvedStruct->name.c_str())) {
          // Parent objects will never be the same in this case.
          assert(resolvedStruct->parentObject);
          assert(other.resolvedStruct->parentObject);
          return _stricmp(resolvedStruct->parentObject->name.c_str(), other.resolvedStruct->parentObject->name.c_str()) != 0;
        }
        return true;
      case Kind::ResolvedObject:
        if (resolvedObject == other.resolvedObject)
          return false;
        return _stricmp(resolvedObject->name.c_str(), other.resolvedObject->name.c_str()) != 0;
    }
    CapricaReportingContext::logicalFatal("Unknown PapyrusTypeKind while comparing!");
  }
  return true;
}

}}
