#include <papyrus/PapyrusResolutionContext.h>

#include <papyrus/PapyrusObject.h>
#include <papyrus/PapyrusStruct.h>

namespace caprica { namespace papyrus {

PapyrusType PapyrusResolutionContext::resolveType(PapyrusType tp) {
  if (tp.type != PapyrusType::Kind::Unresolved) {
    if (tp.type == PapyrusType::Kind::Array) {
      *tp.arrayElementType = resolveType(*tp.arrayElementType);
    }
    return tp;
  }

  if (object) {
    for (auto& s : object->structs) {
      if (!_stricmp(s->name.c_str(), tp.name.c_str())) {
        tp.type = PapyrusType::Kind::ResolvedStruct;
        tp.resolvedStruct = s;
        return tp;
      }
    }

    if (!_stricmp(object->name.c_str(), tp.name.c_str())) {
      tp.type = PapyrusType::Kind::ResolvedObject;
      tp.resolvedObject = object;
      return tp;
    }
  }

  // TODO: Search directory for other objects.
  return tp;
}

PapyrusIdentifier PapyrusResolutionContext::resolveMemberIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const {
  if (ident.type != PapyrusIdentifierType::Unresolved) {
    return ident;
  }

  if (baseType.type == PapyrusType::Kind::ResolvedStruct) {
    for (auto& sm : baseType.resolvedStruct->members) {
      if (!_stricmp(sm->name.c_str(), ident.name.c_str())) {
        PapyrusIdentifier id = ident;
        id.type = PapyrusIdentifierType::StructMember;
        id.structMember = sm;
        return id;
      }
    }
  } else if (baseType.type == PapyrusType::Kind::ResolvedObject) {
    for (auto& propGroup : baseType.resolvedObject->propertyGroups) {
      for (auto& prop : propGroup->properties) {
        if (!_stricmp(prop->name.c_str(), ident.name.c_str())) {
          PapyrusIdentifier id = ident;
          id.type = PapyrusIdentifierType::Property;
          id.prop = prop;
          return id;
        }
      }
    }
  }

  throw std::runtime_error("Unresolved identifier '" + ident.name + "'!");
}

PapyrusIdentifier PapyrusResolutionContext::resolveFunctionIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const {
  if (ident.type != PapyrusIdentifierType::Unresolved) {
    return ident;
  }

  if (baseType == PapyrusType::None()) {
    for (auto& state : object->states) {
      for (auto& func : state->functions) {
        if (!_stricmp(func->name.c_str(), ident.name.c_str())) {
          PapyrusIdentifier id = ident;
          id.type = PapyrusIdentifierType::Function;
          id.func = func;
          return id;
        }
      }
    }
  } else if (baseType.type == PapyrusType::Kind::Array) {
    PapyrusIdentifier id = ident;
    id.type = PapyrusIdentifierType::BuiltinArrayFunction;
    id.arrayFuncElementType = baseType.getElementType();
    if (!_stricmp(ident.name.c_str(), "find")) {
      if (baseType.arrayElementType->type == PapyrusType::Kind::ResolvedStruct)
        id.arrayFuncKind = PapyrusBuiltinArrayFunctionKind::FindStruct;
      else
        id.arrayFuncKind = PapyrusBuiltinArrayFunctionKind::Find;
    } else if (!_stricmp(ident.name.c_str(), "rfind")) {
      if (baseType.arrayElementType->type == PapyrusType::Kind::ResolvedStruct)
        id.arrayFuncKind = PapyrusBuiltinArrayFunctionKind::RFindStruct;
      else
        id.arrayFuncKind = PapyrusBuiltinArrayFunctionKind::RFind;
    } else if (!_stricmp(ident.name.c_str(), "add")) {
      id.arrayFuncKind = PapyrusBuiltinArrayFunctionKind::Add;
    } else if (!_stricmp(ident.name.c_str(), "clear")) {
      id.arrayFuncKind = PapyrusBuiltinArrayFunctionKind::Clear;
    } else if (!_stricmp(ident.name.c_str(), "insert")) {
      id.arrayFuncKind = PapyrusBuiltinArrayFunctionKind::Insert;
    } else if (!_stricmp(ident.name.c_str(), "remove")) {
      id.arrayFuncKind = PapyrusBuiltinArrayFunctionKind::Remove;
    } else if (!_stricmp(ident.name.c_str(), "removelast")) {
      id.arrayFuncKind = PapyrusBuiltinArrayFunctionKind::RemoveLast;
    } else {
      fatalError("Unknown function '" + ident.name + "' called on an array expression!");
    }
    return id;
  } else if (baseType.type == PapyrusType::Kind::ResolvedObject) {
    for (auto& state : baseType.resolvedObject->states) {
      for (auto& func : state->functions) {
        if (!_stricmp(func->name.c_str(), ident.name.c_str())) {
          PapyrusIdentifier id = ident;
          id.type = PapyrusIdentifierType::Function;
          id.func = func;
          return id;
        }
      }
    }
  }

  throw std::runtime_error("Unresolved function name '" + ident.name + "'!");
}

}}
