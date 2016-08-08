#include <papyrus/PapyrusResolutionContext.h>

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <boost/filesystem.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <common/CapricaConfig.h>
#include <common/CapricaReportingContext.h>
#include <common/FSUtils.h>

#include <papyrus/PapyrusCompilationContext.h>
#include <papyrus/PapyrusCustomEvent.h>
#include <papyrus/PapyrusFunction.h>
#include <papyrus/PapyrusObject.h>
#include <papyrus/PapyrusScript.h>
#include <papyrus/PapyrusStruct.h>
#include <papyrus/expressions/PapyrusCastExpression.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/expressions/PapyrusLiteralExpression.h>
#include <papyrus/statements/PapyrusDeclareStatement.h>

namespace caprica { namespace papyrus {

void PapyrusResolutionContext::addImport(const CapricaFileLocation& location, boost::string_ref import) {
  PapyrusCompilationNode* retNode;
  boost::string_ref retStrucName;
  if (!PapyrusCompilationContext::tryFindType(object ? object->getNamespaceName() : "", import, &retNode, &retStrucName))
    reportingContext.error(location, "Failed to find imported script '%s'!", import.to_string().c_str());
  if (retStrucName.length())
    reportingContext.error(location, "You cannot directly import a single struct '%s'!", import.to_string().c_str());
  for (auto o : importedNodes) {
    if (o == retNode)
      reportingContext.error(location, "Duplicate import of '%s'.", import.to_string().c_str());
  }
  importedNodes.push_back(retNode);
}

bool PapyrusResolutionContext::isObjectSomeParentOf(const PapyrusObject* child, const PapyrusObject* parent) {
  if (child == parent)
    return true;
  if (idEq(child->name, parent->name))
    return true;
  if (auto parentObject = child->tryGetParentClass())
    return isObjectSomeParentOf(parentObject, parent);
  return false;
}

bool PapyrusResolutionContext::canExplicitlyCast(const PapyrusType& src, const PapyrusType& dest) {
  if (canImplicitlyCoerce(src, dest))
    return true;

  if (src.type == PapyrusType::Kind::Var)
    return dest.type != PapyrusType::Kind::None;

  switch (dest.type) {
    case PapyrusType::Kind::Int:
    case PapyrusType::Kind::Float:
      switch (src.type) {
        case PapyrusType::Kind::String:
        case PapyrusType::Kind::Int:
        case PapyrusType::Kind::Float:
        case PapyrusType::Kind::Bool:
        case PapyrusType::Kind::Var:
          return true;
        default:
          return false;
      }

    case PapyrusType::Kind::ResolvedObject:
      if (src.type == PapyrusType::Kind::ResolvedObject)
        return isObjectSomeParentOf(dest.resolvedObject, src.resolvedObject);
      return false;
    case PapyrusType::Kind::Array:
      if (src.type == PapyrusType::Kind::Array && src.getElementType().type == PapyrusType::Kind::ResolvedObject && dest.getElementType().type == PapyrusType::Kind::ResolvedObject) {
        return isObjectSomeParentOf(dest.getElementType().resolvedObject, src.getElementType().resolvedObject);
      }
      return false;

    case PapyrusType::Kind::None:
    case PapyrusType::Kind::Bool:
    case PapyrusType::Kind::String:
    case PapyrusType::Kind::Var:
    case PapyrusType::Kind::CustomEventName:
    case PapyrusType::Kind::ScriptEventName:
    case PapyrusType::Kind::Unresolved:
    case PapyrusType::Kind::ResolvedStruct:
      return false;
  }
  CapricaReportingContext::logicalFatal("Unknown PapyrusTypeKind!");
}

bool PapyrusResolutionContext::canImplicitlyCoerce(const PapyrusType& src, const PapyrusType& dest) {
  if (src == dest)
    return true;

  switch (dest.type) {
    case PapyrusType::Kind::Bool:
      return src.type != PapyrusType::Kind::None;
    case PapyrusType::Kind::Float:
      return src.type == PapyrusType::Kind::Int;
    case PapyrusType::Kind::String:
      return src.type != PapyrusType::Kind::None;
    case PapyrusType::Kind::ResolvedObject:
      if (src.type == PapyrusType::Kind::ResolvedObject)
        return isObjectSomeParentOf(src.resolvedObject, dest.resolvedObject);
      return false;
    case PapyrusType::Kind::Var:
      return src.type != PapyrusType::Kind::None && src.type != PapyrusType::Kind::Array;
    case PapyrusType::Kind::None:
    case PapyrusType::Kind::Int:
    case PapyrusType::Kind::Array:
    case PapyrusType::Kind::Unresolved:
    case PapyrusType::Kind::ResolvedStruct:
    case PapyrusType::Kind::CustomEventName:
    case PapyrusType::Kind::ScriptEventName:
      return false;
  }
  CapricaReportingContext::logicalFatal("Unknown PapyrusTypeKind!");
}

bool PapyrusResolutionContext::canImplicitlyCoerceExpression(expressions::PapyrusExpression* expr, const PapyrusType& target) {
  switch (target.type) {
    case PapyrusType::Kind::Var:
    case PapyrusType::Kind::Array:
    case PapyrusType::Kind::ResolvedObject:
    case PapyrusType::Kind::ResolvedStruct:
      // Implicit conversion from None to each of these is allowed, but only for a literal None
      if (expr->resultType().type == PapyrusType::Kind::None && expr->asLiteralExpression()) {
        return true;
      }
      // Deliberate Fallthrough

    case PapyrusType::Kind::Bool:
    case PapyrusType::Kind::Int:
    case PapyrusType::Kind::Float:
    case PapyrusType::Kind::String:
    case PapyrusType::Kind::CustomEventName:
    case PapyrusType::Kind::ScriptEventName:
    case PapyrusType::Kind::Unresolved:
    case PapyrusType::Kind::None:
      return canImplicitlyCoerce(expr->resultType(), target);
  }
  CapricaReportingContext::logicalFatal("Unknown PapyrusTypeKind!");
}

expressions::PapyrusExpression* PapyrusResolutionContext::coerceExpression(expressions::PapyrusExpression* expr, const PapyrusType& target) const {
  if (expr->resultType() != target) {
    bool canCast = canImplicitlyCoerceExpression(expr, target);

    if (canCast && expr->resultType().type == PapyrusType::Kind::Int && target.type == PapyrusType::Kind::Float) {
      if (auto le = expr->asLiteralExpression()) {
        le->value = PapyrusValue::Float(le->value.location, (float)le->value.i);
        return expr;
      }
    }

    if (!canCast) {
      reportingContext.error(expr->location, "No implicit conversion from '%s' to '%s' exists!", expr->resultType().prettyString().c_str(), target.prettyString().c_str());
      return expr;
    }
    auto ce = allocator->make<expressions::PapyrusCastExpression>(expr->location, target);
    ce->innerExpression = expr;
    return ce;
  }
  return expr;
}

PapyrusValue PapyrusResolutionContext::coerceDefaultValue(const PapyrusValue& val, const PapyrusType& target) const {
  if (val.type == PapyrusValueType::Invalid || val.getPapyrusType() == target)
    return val;

  switch (target.type) {
    case PapyrusType::Kind::Float:
      if (val.getPapyrusType().type == PapyrusType::Kind::Int && target.type == PapyrusType::Kind::Float)
        return PapyrusValue::Float(val.location, (float)val.i);
      break;
    case PapyrusType::Kind::Array:
    case PapyrusType::Kind::ResolvedObject:
    case PapyrusType::Kind::ResolvedStruct:
      if (val.getPapyrusType().type == PapyrusType::Kind::None)
        return val;
      break;
    default:
      break;
  }
  reportingContext.error(val.location, "Cannot initialize a '%s' value with a '%s'!", target.prettyString().c_str(), val.getPapyrusType().prettyString().c_str());
  return val;
}

void PapyrusResolutionContext::checkForPoison(const expressions::PapyrusExpression* expr) const {
  checkForPoison(expr->resultType());
}

void PapyrusResolutionContext::checkForPoison(const PapyrusType& type) const {
  if (type.isPoisoned(PapyrusType::PoisonKind::Beta)) {
    if (function != nullptr && function->isBetaOnly())
      goto CheckDebug;
    if (object != nullptr && object->isBetaOnly())
      goto CheckDebug;
    reportingContext.error(type.location, "You cannot use the return value of a BetaOnly function in a non-BetaOnly context!");
    return;
  }
CheckDebug:
  if (type.isPoisoned(PapyrusType::PoisonKind::Debug)) {
    if (function != nullptr && function->isDebugOnly())
      return;
    if (object != nullptr && object->isDebugOnly())
      return;
    reportingContext.error(type.location, "You cannot use the return value of a DebugOnly function in a non-DebugOnly context!");
    return;
  }
}

const PapyrusFunction* PapyrusResolutionContext::tryResolveEvent(const PapyrusObject* parentObj, boost::string_ref name) const {
  for (auto f : parentObj->getRootState()->functions) {
    if (idEq(f->name, name) && f->functionType == PapyrusFunctionType::Event)
      return f;
  }

  if (auto parentClass = parentObj->tryGetParentClass())
    return tryResolveEvent(parentClass, name);

  return nullptr;
}

const PapyrusCustomEvent* PapyrusResolutionContext::tryResolveCustomEvent(const PapyrusObject* parentObj, boost::string_ref name) const {
  for (auto c : parentObj->customEvents) {
    if (idEq(c->name, name))
      return c;
  }

  if (auto parentClass = parentObj->tryGetParentClass())
    return tryResolveCustomEvent(parentClass, name);

  return nullptr;
}

const PapyrusState* PapyrusResolutionContext::tryResolveState(boost::string_ref name, const PapyrusObject* parentObj) const {
  if (!parentObj)
    parentObj = object;

  for (auto s : parentObj->states) {
    if (idEq(s->name, name))
      return s;
  }

  if (auto parentClass = parentObj->tryGetParentClass())
    return tryResolveState(name, parentClass);

  return nullptr;
}

static bool tryResolveStruct(const PapyrusObject* object, boost::string_ref structName, const PapyrusStruct** ret) {
  for (auto& s : object->structs) {
    if (idEq(s->name, structName)) {
      *ret = s;
      return true;
    }
  }

  if (auto parentClass = object->tryGetParentClass())
    return tryResolveStruct(parentClass, structName, ret);

  return false;
}

PapyrusType PapyrusResolutionContext::resolveType(PapyrusType tp, bool lazy) {
  if (tp.type != PapyrusType::Kind::Unresolved) {
    if (tp.type == PapyrusType::Kind::Array && tp.getElementType().type == PapyrusType::Kind::Unresolved)
      return PapyrusType::Array(tp.location, allocator->make<PapyrusType>(resolveType(tp.getElementType(), lazy)));
    return tp;
  }

  /*if (isPexResolution || conf::Papyrus::allowDecompiledStructNameRefs) {
    auto pos = tp.name.find('#');
    if (pos != std::string::npos) {
      auto scName = tp.name.substr(0, pos);
      auto strucName = tp.name.substr(pos + 1);
      auto sc = PapyrusScriptLoader::loadScript(scName, scName, "", PapyrusScriptLoader::LoadType::Reference);
      if (!sc)
        reportingContext.fatal(tp.location, "Unable to find script '%s' referenced by '%s'!", scName.c_str(), tp.name.c_str());

      for (auto obj : sc->objects) {
        for (auto struc : obj->structs) {
          if (idEq(struc->name, strucName))
            return PapyrusType::ResolvedStruct(tp.location, struc);
        }
      }

      reportingContext.fatal(tp.location, "Unable to resolve a struct named '%s' in script '%s'!", strucName.c_str(), scName.c_str());
    }
  }*/

  if (object) {
    const PapyrusStruct* struc = nullptr;
    if (tryResolveStruct(object, tp.name, &struc))
      return PapyrusType::ResolvedStruct(tp.location, struc);

    if (idEq(object->name, tp.name))
      return PapyrusType::ResolvedObject(tp.location, object);
  }

  for (auto node : importedNodes) {
    auto obj = lazy ? node->awaitParse() : node->awaitSemantic();
    for (auto struc : obj->structs) {
      if (idEq(struc->name, tp.name))
        return PapyrusType::ResolvedStruct(tp.location, struc);
    }
  }

  PapyrusCompilationNode* retNode{ nullptr };
  boost::string_ref retStructName;
  if (!PapyrusCompilationContext::tryFindType(object ? object->getNamespaceName() : "", tp.name, &retNode, &retStructName))
    reportingContext.fatal(tp.location, "Unable to resolve type '%s'!", tp.name.to_string().c_str());

  PapyrusObject* foundObj = lazy ? retNode->awaitParse() : retNode->awaitSemantic();
  if (retStructName.size() == 0)
    return PapyrusType::ResolvedObject(tp.location, foundObj);

  const PapyrusStruct* resStruct = nullptr;
  if (tryResolveStruct(foundObj, retStructName, &resStruct))
    return PapyrusType::ResolvedStruct(tp.location, resStruct);
  reportingContext.fatal(tp.location, "Unable to resolve a struct named '%s' in script '%s'!", retStructName.to_string().c_str(), foundObj->name.to_string().c_str());
}

void PapyrusResolutionContext::addLocalVariable(statements::PapyrusDeclareStatement* local) {
  for (auto is : localVariableScopeStack) {
    if (is.count(local->name)) {
      reportingContext.error(local->location, "Attempted to redefined '%s' which was already defined in a parent scope!", local->name.to_string().c_str());
      return;
    }
  }
  localVariableScopeStack.back().emplace(local->name, local);
}

PapyrusIdentifier PapyrusResolutionContext::resolveIdentifier(const PapyrusIdentifier& ident) const {
  auto id = tryResolveIdentifier(ident);
  if (id.type == PapyrusIdentifierType::Unresolved)
    reportingContext.fatal(ident.location, "Unresolved identifier '%s'!", ident.name.to_string().c_str());
  return id;
}

PapyrusIdentifier PapyrusResolutionContext::tryResolveIdentifier(const PapyrusIdentifier& ident) const {
  if (ident.type != PapyrusIdentifierType::Unresolved)
    return ident;

  // This handles local var resolution.
  for (auto& stack : boost::adaptors::reverse(localVariableScopeStack)) {
    auto f = stack.find(ident.name);
    if (f != stack.end()) {
      return PapyrusIdentifier::DeclStatement(ident.location, f->second);
    }
  }

  if (function) {
    if ((idEq(function->name, "getstate") || idEq(function->name, "gotostate")) && idEq(ident.name, "__state")) {
      auto i = ident;
      i.type = PapyrusIdentifierType::BuiltinStateField;
      return i;
    }

    for (auto p : function->parameters) {
      if (idEq(p->name, ident.name))
        return PapyrusIdentifier::FunctionParameter(ident.location, p);
    }
  }

  if (!function || !function->isGlobal()) {
    for (auto v : object->variables) {
      if (idEq(v->name, ident.name))
        return PapyrusIdentifier::Variable(ident.location, v);
    }

    for (auto pg : object->propertyGroups) {
      for (auto p : pg->properties) {
        if (idEq(p->name, ident.name))
          return PapyrusIdentifier::Property(ident.location, p);
      }
    }
  }

  if (auto parentClass = object->tryGetParentClass())
    return tryResolveMemberIdentifier(object->parentClass, ident);

  return ident;
}

PapyrusIdentifier PapyrusResolutionContext::resolveMemberIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const {
  auto id = tryResolveMemberIdentifier(baseType, ident);
  if (id.type == PapyrusIdentifierType::Unresolved)
    reportingContext.fatal(ident.location, "Unresolved identifier '%s'!", ident.name.to_string().c_str());
  return id;
}

PapyrusIdentifier PapyrusResolutionContext::tryResolveMemberIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const {
  if (ident.type != PapyrusIdentifierType::Unresolved)
    return ident;

  if (baseType.type == PapyrusType::Kind::ResolvedStruct) {
    baseType.resolvedStruct->parentObject->awaitSemantic();
    for (auto& sm : baseType.resolvedStruct->members) {
      if (idEq(sm->name, ident.name))
        return PapyrusIdentifier::StructMember(ident.location, sm);
    }
  } else if (baseType.type == PapyrusType::Kind::ResolvedObject) {
    for (auto& propGroup : baseType.resolvedObject->awaitSemantic()->propertyGroups) {
      for (auto& prop : propGroup->properties) {
        if (idEq(prop->name, ident.name))
          return PapyrusIdentifier::Property(ident.location, prop);
      }
    }

    if (auto parentClass = baseType.resolvedObject->tryGetParentClass())
      return tryResolveMemberIdentifier(baseType.resolvedObject->parentClass, ident);
  }

  return ident;
}

PapyrusIdentifier PapyrusResolutionContext::resolveFunctionIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident, bool wantGlobal) const {
  auto id = tryResolveFunctionIdentifier(baseType, ident, wantGlobal);
  if (id.type == PapyrusIdentifierType::Unresolved)
    reportingContext.fatal(ident.location, "Unresolved function name '%s'!", ident.name.to_string().c_str());
  return id;
}

PapyrusIdentifier PapyrusResolutionContext::tryResolveFunctionIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident, bool wantGlobal) const {
  wantGlobal = wantGlobal || (function && function->isGlobal());
  if (ident.type != PapyrusIdentifierType::Unresolved)
    return ident;

  if (baseType.type == PapyrusType::Kind::None) {
    if (auto state = object->getRootState()) {
      for (auto& func : state->functions) {
        if (idEq(func->name, ident.name)) {
          if (wantGlobal && !func->isGlobal())
            reportingContext.error(ident.location, "You cannot call non-global functions from within a global function. '%s' is not a global function.", func->name.to_string().c_str());
          return PapyrusIdentifier::Function(ident.location, func);
        }
      }
    }

    for (auto node : importedNodes) {
      if (auto state = node->awaitSemantic()->getRootState()) {
        for (auto& func : state->functions) {
          if (func->isGlobal() && idEq(func->name, ident.name))
            return PapyrusIdentifier::Function(ident.location, func);
        }
      }
    }

    return tryResolveFunctionIdentifier(PapyrusType::ResolvedObject(ident.location, object), ident, wantGlobal);
  } else if (baseType.type == PapyrusType::Kind::Array) {
    auto fk = PapyrusBuiltinArrayFunctionKind::Unknown;
    if (idEq(ident.name, "find")) {
      fk = PapyrusBuiltinArrayFunctionKind::Find;
    } else if(idEq(ident.name, "findstruct")) {
      fk = PapyrusBuiltinArrayFunctionKind::FindStruct;
    } else if (idEq(ident.name, "rfind")) {
      fk = PapyrusBuiltinArrayFunctionKind::RFind;
    } else if (idEq(ident.name, "rfindstruct")) {
      fk = PapyrusBuiltinArrayFunctionKind::RFindStruct;
    } else if (idEq(ident.name, "add")) {
      fk = PapyrusBuiltinArrayFunctionKind::Add;
    } else if (idEq(ident.name, "clear")) {
      fk = PapyrusBuiltinArrayFunctionKind::Clear;
    } else if (idEq(ident.name, "insert")) {
      fk = PapyrusBuiltinArrayFunctionKind::Insert;
    } else if (idEq(ident.name, "remove")) {
      fk = PapyrusBuiltinArrayFunctionKind::Remove;
    } else if (idEq(ident.name, "removelast")) {
      fk = PapyrusBuiltinArrayFunctionKind::RemoveLast;
    } else {
      reportingContext.fatal(ident.location, "Unknown function '%s' called on an array expression!", ident.name.to_string().c_str());
    }
    return PapyrusIdentifier::ArrayFunction(baseType.location, fk, allocator->make<PapyrusType>(baseType.getElementType()));
  } else if (baseType.type == PapyrusType::Kind::ResolvedObject) {
    if (auto state = baseType.resolvedObject->awaitSemantic()->getRootState()) {
      for (auto& func : state->functions) {
        if (idEq(func->name, ident.name)) {
          if (!wantGlobal && func->isGlobal())
            reportingContext.error(ident.location, "You cannot call the global function '%s' on an object.", func->name.to_string().c_str());
          return PapyrusIdentifier::Function(ident.location, func);
        }
      }
    }

    if (auto parentClass = baseType.resolvedObject->tryGetParentClass())
      return tryResolveFunctionIdentifier(baseType.resolvedObject->parentClass, ident, wantGlobal);
  }

  return ident;
}

}}
