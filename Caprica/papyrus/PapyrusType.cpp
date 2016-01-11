#include <papyrus/PapyrusType.h>

#include <boost/algorithm/string/case_conv.hpp>

#include <papyrus/PapyrusObject.h>
#include <papyrus/PapyrusStruct.h>

namespace caprica { namespace papyrus {

std::string PapyrusType::prettyString() const {
  switch (type) {
    case Kind::None:
      return "none";
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
    case Kind::Array:
      return arrayElementType->prettyString() + "[]";
    case Kind::Unresolved:
      return "unresolved(" + name + ")";
    case Kind::ResolvedObject:
      return resolvedObject->name;
    case Kind::ResolvedStruct:
      return "struct " + resolvedStruct->parentObject->name + "." + resolvedStruct->name;
    default:
      CapricaError::logicalFatal("Unknown PapyrusTypeKind!");
  }
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
    default:
      CapricaError::logicalFatal("Unknown PapyrusTypeKind!");
  }
}

}}
