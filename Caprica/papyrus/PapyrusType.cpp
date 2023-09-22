#include <papyrus/PapyrusType.h>

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
      return resolved.arrayElementType->prettyString() + "[]";
    case Kind::Unresolved:
      return "unresolved(" + name.to_string() + ")";
    case Kind::ResolvedObject:
      return resolved.obj->name.to_string();
    case Kind::ResolvedStruct:
      return "struct " + resolved.struc->parentObject->name.to_string() + "." + resolved.struc->name.to_string();
  }
  CapricaReportingContext::logicalFatal("Unknown PapyrusTypeKind!");
}

LargelyBufferedString& PapyrusType::getTypeStringAsRef(LargelyBufferedString& buf) const {
  switch (type) {
    case Kind::None:
      return buf.append("None");
    case Kind::Bool:
      return buf.append("Bool");
    case Kind::Float:
      return buf.append("Float");
    case Kind::Int:
      return buf.append("Int");
    case Kind::String:
    case Kind::CustomEventName: // These are both really strings.
    case Kind::ScriptEventName:
      return buf.append("String");
    case Kind::Var:
      return buf.append("Var");
    case Kind::Array:
      return resolved.arrayElementType->getTypeStringAsRef(buf).append("[]");
    case Kind::Unresolved:
      return buf.append(name.to_string_view());
    case Kind::ResolvedObject:
      return buf.append(resolved.obj->loweredName().to_string_view());
    case Kind::ResolvedStruct: {
      auto& typeName = buf.append(resolved.struc->parentObject->name.to_string_view())
                           .append("#")
                           .append(resolved.struc->name.to_string_view());
      identifierToLower(typeName.data(), typeName.size());
      return typeName;
    }
  }
  CapricaReportingContext::logicalFatal("Unknown PapyrusTypeKind!");
}

bool PapyrusType::operator!=(const PapyrusType& other) const {
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
        return *resolved.arrayElementType != *other.resolved.arrayElementType;
      case Kind::Unresolved:
        return !idEq(name, other.name);
      case Kind::ResolvedStruct:
        if (resolved.struc == other.resolved.struc)
          return false;
        if (idEq(resolved.struc->name, other.resolved.struc->name)) {
          // Parent objects will never be the same in this case.
          assert(resolved.struc->parentObject);
          assert(other.resolved.struc->parentObject);
          return !idEq(resolved.struc->parentObject->name, other.resolved.struc->parentObject->name);
        }
        return true;
      case Kind::ResolvedObject:
        if (resolved.obj == other.resolved.obj)
          return false;
        return !idEq(resolved.obj->name, other.resolved.obj->name);
    }
    CapricaReportingContext::logicalFatal("Unknown PapyrusTypeKind while comparing!");
  }
  return true;
}

}}
