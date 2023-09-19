#include <papyrus/PapyrusResolutionContext.h>

#include <memory>
#include <unordered_map>
#include <unordered_set>

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

void PapyrusResolutionContext::addImport(const CapricaFileLocation& location, const identifier_ref& import) {
  PapyrusCompilationNode* retNode;
  identifier_ref retStrucName;
  if (!PapyrusCompilationContext::tryFindType(object ? object->getNamespaceName() : "", import, &retNode, &retStrucName))
    reportingContext.error(location, "Failed to find imported script '%s'!", import.to_string().c_str());
  if (retStrucName.size())
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

bool PapyrusResolutionContext::canExplicitlyCast(CapricaFileLocation loc, const PapyrusType& src, const PapyrusType& dest) const {
  if (canImplicitlyCoerce(loc, src, dest))
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
        return isObjectSomeParentOf(dest.resolved.obj, src.resolved.obj);
      return false;
    case PapyrusType::Kind::Array:
      if (src.type == PapyrusType::Kind::Array && src.getElementType().type == PapyrusType::Kind::ResolvedObject && dest.getElementType().type == PapyrusType::Kind::ResolvedObject) {
        // TODO: New in starfield, downcasting arrays of objects is allowed if explicit (VERIFY)
        if (isObjectSomeParentOf(dest.getElementType().resolved.obj, src.getElementType().resolved.obj)){
          return true;
        }
        if (isObjectSomeParentOf(src.getElementType().resolved.obj, dest.getElementType().resolved.obj)){
          reportingContext.warning_W6004_Experimental_Downcast_Arrays(
                  loc,
                  (src.getElementType().resolved.obj->name.to_string() + "[]").c_str(),
                  (dest.getElementType().resolved.obj->name.to_string() + "[]").c_str());
          return true;
        }
        return  false;
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

bool PapyrusResolutionContext::canImplicitlyCoerce(CapricaFileLocation loc, const PapyrusType& src, const PapyrusType& dest) const {
  if (src == dest)
    return true;

  if (src.type == PapyrusType::Kind::None) {
    if (conf::Papyrus::allowImplicitNoneCastsToAnyType){
      return true;
    }
    switch (dest.type) {
      case PapyrusType::Kind::Bool:
      case PapyrusType::Kind::ResolvedObject:
      case PapyrusType::Kind::ResolvedStruct:
      case PapyrusType::Kind::Var:
      case PapyrusType::Kind::String:
      case PapyrusType::Kind::Array:
        reportingContext.warning_W1003_Strict_None_Implicit_Conversion(loc, dest.prettyString().c_str());
        return true;
      default:
        break;
    }
    reportingContext.error(loc, "Cannot convert None to '%s'!", dest.prettyString().c_str());
    return false;
  }

  switch (dest.type) {
    case PapyrusType::Kind::Bool:
      return src.type != PapyrusType::Kind::None;
    case PapyrusType::Kind::Float:
      return src.type == PapyrusType::Kind::Int;
    case PapyrusType::Kind::String:
      return src.type != PapyrusType::Kind::None;
    case PapyrusType::Kind::ResolvedObject:
      if (src.type == PapyrusType::Kind::ResolvedObject)
        return isObjectSomeParentOf(src.resolved.obj, dest.resolved.obj);
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

bool PapyrusResolutionContext::canImplicitlyCoerceExpression(expressions::PapyrusExpression* expr, const PapyrusType& target) const {
  switch (target.type) {
    case PapyrusType::Kind::Var:
    case PapyrusType::Kind::Array:
    case PapyrusType::Kind::ResolvedObject:
    case PapyrusType::Kind::ResolvedStruct:
      // Implicit conversion from None to each of these is allowed, but only for a literal None
      if (expr->resultType().type == PapyrusType::Kind::None && expr->asLiteralExpression()) {
        return true;
      }
      return canImplicitlyCoerce(expr->location, expr->resultType(), target);

    case PapyrusType::Kind::Bool:
    case PapyrusType::Kind::Int:
    case PapyrusType::Kind::Float:
    case PapyrusType::Kind::String:
    case PapyrusType::Kind::CustomEventName:
    case PapyrusType::Kind::ScriptEventName:
    case PapyrusType::Kind::Unresolved:
    case PapyrusType::Kind::None:
      return canImplicitlyCoerce(expr->location, expr->resultType(), target);
  }
  CapricaReportingContext::logicalFatal("Unknown PapyrusTypeKind!");
}

expressions::PapyrusExpression* PapyrusResolutionContext::coerceExpression(expressions::PapyrusExpression* expr, const PapyrusType& target) const {
  if (expr->resultType() != target) {
    bool canCast = canImplicitlyCoerceExpression(expr, target);

    if (canCast && expr->resultType().type == PapyrusType::Kind::Int && target.type == PapyrusType::Kind::Float) {
      if (auto le = expr->asLiteralExpression()) {
        le->value = PapyrusValue::Float(le->value.location, (float)le->value.val.i);
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
        return PapyrusValue::Float(val.location, (float)val.val.i);
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
    reportingContext.warning_W1001_Strict_Poison_BetaOnly(type.location);
    return;
  }
CheckDebug:
  if (type.isPoisoned(PapyrusType::PoisonKind::Debug)) {
    if (function != nullptr && function->isDebugOnly())
      return;
    if (object != nullptr && object->isDebugOnly())
      return;
    reportingContext.warning_W1002_Strict_Poison_DebugOnly(type.location);
    return;
  }
}

const PapyrusFunction* PapyrusResolutionContext::tryResolveEvent(const PapyrusObject* parentObj, const identifier_ref& name) const {
  auto func = parentObj->getRootState()->functions.find(name);
  if (func != parentObj->getRootState()->functions.end() && func->second->functionType == PapyrusFunctionType::Event) {
    return func->second;
  }

  if (auto parentClass = parentObj->tryGetParentClass())
    return tryResolveEvent(parentClass, name);

  return nullptr;
}

const PapyrusCustomEvent* PapyrusResolutionContext::tryResolveCustomEvent(const PapyrusObject* parentObj, const identifier_ref& name) const {
  for (auto c : parentObj->customEvents) {
    if (idEq(c->name, name))
      return c;
  }

  if (auto parentClass = parentObj->tryGetParentClass())
    return tryResolveCustomEvent(parentClass, name);

  return nullptr;
}

const PapyrusState* PapyrusResolutionContext::tryResolveState(const identifier_ref& name, const PapyrusObject* parentObj) const {
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

const PapyrusGuard* PapyrusResolutionContext::tryResolveGuard(const PapyrusObject* parentObject, const identifier_ref& guardName) {
  // TODO: Starfield: Verify that guards are not inherited and limited to the current parentObject.
  for (auto &s: parentObject->guards) {
    if (idEq(s->name, guardName)) {
      return s;
    }
  }
  return nullptr;
}

static bool tryResolveStruct(const PapyrusObject* object, const identifier_ref& structName, const PapyrusStruct** ret) {
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
  identifier_ref retStructName;
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
    for (auto n : is->locals) {
      if (idEq(n->name, local->name)) {
        reportingContext.error(local->location, "Attempted to redefined '%s' which was already defined in a parent scope!", local->name.to_string().c_str());
        return;
      }
    }
  }
  localVariableScopeStack.top()->locals.push_back(allocator->make<LocalScopeVariableNode>(local->name, local));
}

PapyrusIdentifier PapyrusResolutionContext::resolveIdentifier(const PapyrusIdentifier& ident) const {
  auto id = tryResolveIdentifier(ident);
  if (id.type == PapyrusIdentifierType::Unresolved)
    reportingContext.fatal(ident.location, "Unresolved identifier '%s'!", ident.res.name.to_string().c_str());
  return id;
}

PapyrusIdentifier PapyrusResolutionContext::tryResolveIdentifier(const PapyrusIdentifier& ident) const {
  if (ident.type != PapyrusIdentifierType::Unresolved)
    return ident;
  bool ignoreConflicts = conf::Papyrus::ignorePropertyNameLocalConflicts;
  std::vector<PapyrusIdentifier> resolvedIds;

  // This handles local var resolution.
  for (auto stack : localVariableScopeStack) {
    for (auto n : stack->locals) {
      if (idEq(n->name, ident.res.name)) {
        resolvedIds.push_back(PapyrusIdentifier::DeclStatement(ident.location, n->declareStatement));
        if (ignoreConflicts) { return resolvedIds[0]; }
      }
    }
  }


  if (function) {
    if ((idEq(function->name, "getstate") || idEq(function->name, "gotostate")) && idEq(ident.res.name, "__state")) {
      PapyrusIdentifier i = ident;
      i.type = PapyrusIdentifierType::BuiltinStateField;
        resolvedIds.push_back(i);
      if (ignoreConflicts) { return resolvedIds[0]; }
    }

    // Parameters are allowed to have the same name as properties
    if (!function->isGlobal()) {
      for (auto pg: object->propertyGroups) {
        for (auto p: pg->properties) {
          if (idEq(p->name, ident.res.name)) {
            resolvedIds.push_back(PapyrusIdentifier::Property(ident.location, p));
            if (ignoreConflicts) { return resolvedIds[0]; }
          }
        }
      }
    }

    for (auto p : function->parameters) {
      if (idEq(p->name, ident.res.name)) {
        resolvedIds.push_back(PapyrusIdentifier::FunctionParameter(ident.location, p));
        if (ignoreConflicts) { return resolvedIds[0]; }
      }
    }

  }

  if (!function || !function->isGlobal()) {
    for (auto v : object->variables) {
      if (idEq(v->name, ident.res.name)) {
        resolvedIds.push_back(PapyrusIdentifier::Variable(ident.location, v));
        if (ignoreConflicts) { return resolvedIds[0]; }
      }
    }

    for (auto g : object->guards) {
      if (idEq(g->name, ident.res.name)) {
        resolvedIds.push_back(PapyrusIdentifier::Guard(ident.location, g));
        if (ignoreConflicts) { return resolvedIds[0]; }
      }
    }

  }

  if (auto parentClass = object->tryGetParentClass()) {
    resolvedIds.push_back(tryResolveMemberIdentifier(object->parentClass, ident));
    if (ignoreConflicts) { return resolvedIds[0]; }
  }

  if (resolvedIds.empty())
    return ident;

  if (resolvedIds.size() == 1) {
    return resolvedIds[0];
  }
  if (resolvedIds.size() > 1) {
    if (resolvedIds[0].type == PapyrusIdentifierType::Property){
      for (auto shadow_ident: resolvedIds) {
        if (shadow_ident.type == PapyrusIdentifierType::Unresolved || shadow_ident.type == PapyrusIdentifierType::Property)
          continue;
        if (shadow_ident.type == PapyrusIdentifierType::Parameter) {
          // TODO: verify that the
          reportingContext.warning_W4008_Function_Parameter_Shadows_Property(shadow_ident.location, PapyrusIdentifier::TypeToString(shadow_ident.type));
        }
      }
    }
    // TODO: object variables vs local/params?
  }
  return resolvedIds[0];
}

PapyrusIdentifier PapyrusResolutionContext::resolveMemberIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const {
  auto id = tryResolveMemberIdentifier(baseType, ident);
  if (id.type == PapyrusIdentifierType::Unresolved)
    reportingContext.fatal(ident.location, "Unresolved identifier '%s'!", ident.res.name.to_string().c_str());
  return id;
}

PapyrusIdentifier PapyrusResolutionContext::tryResolveMemberIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const {
  if (ident.type != PapyrusIdentifierType::Unresolved)
    return ident;

  if (baseType.type == PapyrusType::Kind::ResolvedStruct) {
    baseType.resolved.struc->parentObject->awaitSemantic();
    for (auto& sm : baseType.resolved.struc->members) {
      if (idEq(sm->name, ident.res.name))
        return PapyrusIdentifier::StructMember(ident.location, sm);
    }
  } else if (baseType.type == PapyrusType::Kind::ResolvedObject) {
    for (auto& propGroup : baseType.resolved.obj->awaitSemantic()->propertyGroups) {
      for (auto& prop : propGroup->properties) {
        if (idEq(prop->name, ident.res.name))
          return PapyrusIdentifier::Property(ident.location, prop);
      }
    }
    //TODO: Starfield: Verify that child classes cannot use guards inherited from parent classes.

    if (auto parentClass = baseType.resolved.obj->tryGetParentClass())
      return tryResolveMemberIdentifier(baseType.resolved.obj->parentClass, ident);
  }

  return ident;
}

static const caseless_unordered_identifier_ref_set skyrim_builtin_script_names = {
        "GetState",
        "GotoState"
};

PapyrusIdentifier PapyrusResolutionContext::resolveFunctionIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident, bool wantGlobal) const {
  auto id = tryResolveFunctionIdentifier(baseType, ident, wantGlobal);
  if (id.type == PapyrusIdentifierType::Unresolved)
    reportingContext.fatal(ident.location, "Unresolved function name '%s'!", ident.res.name.to_string().c_str());
  return id;
}

PapyrusIdentifier PapyrusResolutionContext::tryResolveFunctionIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident, bool wantGlobal) const {
  wantGlobal = wantGlobal || (function && function->isGlobal());
  if (ident.type != PapyrusIdentifierType::Unresolved)
    return ident;

  if (baseType.type == PapyrusType::Kind::None) {
    if (auto rootState = object->getRootState()) {
      auto func = rootState->functions.find(ident.res.name);
      if (func != rootState->functions.end()) {
        if (wantGlobal && !func->second->isGlobal())
          reportingContext.error(ident.location, "You cannot call non-global functions from within a global function. '%s' is not a global function.", func->second->name.to_string().c_str());
        return PapyrusIdentifier::Function(ident.location, func->second);
      }
    }

    for (auto node : importedNodes) {
      if (auto rootState = node->awaitSemantic()->getRootState()) {
        auto func = rootState->functions.find(ident.res.name);
        if (func != rootState->functions.end()) {
          if (func->second->isGlobal() && idEq(func->second->name, ident.res.name))
            return PapyrusIdentifier::Function(ident.location, func->second);
        }
      }
    }

    return tryResolveFunctionIdentifier(PapyrusType::ResolvedObject(ident.location, object), ident, wantGlobal);
  } else if (baseType.type == PapyrusType::Kind::Array) {
    auto fk = PapyrusBuiltinArrayFunctionKind::Unknown;
    if (idEq(ident.res.name, "find")) {
      fk = PapyrusBuiltinArrayFunctionKind::Find;
    } else if(idEq(ident.res.name, "findstruct")) {
      fk = PapyrusBuiltinArrayFunctionKind::FindStruct;
    } else if (idEq(ident.res.name, "rfind")) {
      fk = PapyrusBuiltinArrayFunctionKind::RFind;
    } else if (idEq(ident.res.name, "rfindstruct")) {
      fk = PapyrusBuiltinArrayFunctionKind::RFindStruct;
    } else if (idEq(ident.res.name, "add")) {
      fk = PapyrusBuiltinArrayFunctionKind::Add;
    } else if (idEq(ident.res.name, "clear")) {
      fk = PapyrusBuiltinArrayFunctionKind::Clear;
    } else if (idEq(ident.res.name, "insert")) {
      fk = PapyrusBuiltinArrayFunctionKind::Insert;
    } else if (idEq(ident.res.name, "remove")) {
      fk = PapyrusBuiltinArrayFunctionKind::Remove;
    } else if (idEq(ident.res.name, "removelast")) {
      fk = PapyrusBuiltinArrayFunctionKind::RemoveLast;
    } else if (idEq(ident.res.name, "getmatchingstructs")) {
      fk = PapyrusBuiltinArrayFunctionKind::GetMatchingStructs;
      reportingContext.warning_W6001_Experimental_Syntax_ArrayGetAllMatchingStructs(ident.location);
    } else {
      reportingContext.fatal(ident.location, "Unknown function '%s' called on an array expression!", ident.res.name.to_string().c_str());
    }
    if(!isArrayFunctionInGame(fk, conf::Papyrus::game)){
      reportingContext.fatal(ident.location, "Array function '%s' is not available in %s scripts!", ident.res.name.to_string().c_str(), GameIDToString(conf::Papyrus::game));
    }
    return PapyrusIdentifier::ArrayFunction(baseType.location, fk, allocator->make<PapyrusType>(baseType.getElementType()));
  } else if (baseType.type == PapyrusType::Kind::ResolvedObject) {
    if (auto rootState = baseType.resolved.obj->awaitSemantic()->getRootState()) {
      auto func = rootState->functions.find(ident.res.name);
      if (func != rootState->functions.end()) {
        if (!wantGlobal && func->second->isGlobal())
          reportingContext.error(ident.location, "You cannot call the global function '%s' on an object.", func->second->name.to_string().c_str());
        return PapyrusIdentifier::Function(ident.location, func->second);
      }
    }

    if (auto parentClass = baseType.resolved.obj->tryGetParentClass())
      return tryResolveFunctionIdentifier(baseType.resolved.obj->parentClass, ident, wantGlobal);
  }

  return ident;
}

}}
