#include <papyrus/PapyrusType.h>

#include <boost/algorithm/string/case_conv.hpp>

#include <papyrus/PapyrusObject.h>
#include <papyrus/PapyrusStruct.h>

namespace caprica { namespace papyrus {

std::string PapyrusType::getTypeString() const {
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
