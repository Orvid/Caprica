#include <papyrus/PapyrusResolutionContext.h>

#include <memory>

#include <boost/filesystem.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <common/CapricaConfig.h>
#include <common/CapricaError.h>
#include <papyrus/PapyrusObject.h>
#include <papyrus/PapyrusScript.h>
#include <papyrus/PapyrusStruct.h>
#include <papyrus/expressions/PapyrusCastExpression.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/expressions/PapyrusLiteralExpression.h>
#include <papyrus/parser/PapyrusParser.h>
#include <pex/PexReflector.h>
#include <pex/parser/PexAsmParser.h>

namespace caprica { namespace papyrus {

void PapyrusResolutionContext::addImport(const CapricaFileLocation& location, const std::string& import) {
  auto sc = loadScript(import);
  if (!sc)
    CapricaError::fatal(location, "Failed to find imported script '%s.psc'!", import.c_str());
  importedScripts.push_back(sc);
}

// This is safe because it will only ever contain scripts referencing items in this map, and this map
// will never contain a fully-resolved script.
static thread_local std::map<const std::string, std::unique_ptr<PapyrusScript>, CaselessStringComparer> loadedScripts{ };
PapyrusScript* PapyrusResolutionContext::loadScript(const std::string& name) {
  auto f = loadedScripts.find(name);
  if (f != loadedScripts.end())
    return f->second.get();

  for (auto& dir : CapricaConfig::importDirectories) {
    if (boost::filesystem::exists(dir + name + ".psc")) {
      auto parser = new parser::PapyrusParser(dir + name + ".psc");
      auto a = parser->parseScript();
      CapricaError::exitIfErrors();
      delete parser;
      loadedScripts.insert({ a->objects[0]->name, std::unique_ptr<PapyrusScript>(a) });
      auto ctx = new PapyrusResolutionContext();
      ctx->resolvingReferenceScript = true;
      a->semantic(ctx);
      CapricaError::exitIfErrors();
      delete ctx;
      return a;
    } else if (boost::filesystem::exists(dir + name + ".pas")) {
      auto parser = new pex::parser::PexAsmParser(dir + name + ".pas");
      auto pex = parser->parseFile();
      CapricaError::exitIfErrors();
      delete parser;
      auto a = pex::PexReflector::reflectScript(pex);
      CapricaError::exitIfErrors();
      delete pex;
      loadedScripts.insert({ a->objects[0]->name, std::unique_ptr<PapyrusScript>(a) });
      auto ctx = new PapyrusResolutionContext();
      ctx->resolvingReferenceScript = true;
      ctx->isPexResolution = true;
      a->semantic(ctx);
      CapricaError::exitIfErrors();
      delete ctx;
      return a;
    } else if (boost::filesystem::exists(dir + name + ".pex")) {
      pex::PexReader rdr(dir + name + ".pex");
      auto pex = pex::PexFile::read(rdr);
      CapricaError::exitIfErrors();
      auto a = pex::PexReflector::reflectScript(pex);
      CapricaError::exitIfErrors();
      delete pex;
      loadedScripts.insert({ a->objects[0]->name, std::unique_ptr<PapyrusScript>(a) });
      auto ctx = new PapyrusResolutionContext();
      ctx->resolvingReferenceScript = true;
      ctx->isPexResolution = true;
      a->semantic(ctx);
      CapricaError::exitIfErrors();
      delete ctx;
      return a;
    }
  }

  return nullptr;
}

bool PapyrusResolutionContext::isObjectSomeParentOf(const PapyrusObject* child, const PapyrusObject* parent) {
  if (child == parent)
    return true;
  if (!_stricmp(child->name.c_str(), parent->name.c_str()))
    return true;
  if (child->parentClass.type != PapyrusType::Kind::None) {
    assert(child->parentClass.type == PapyrusType::Kind::ResolvedObject);
    return isObjectSomeParentOf(child->parentClass.resolvedObject, parent);
  }
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
    case PapyrusType::Kind::Unresolved:
    case PapyrusType::Kind::ResolvedStruct:
      return false;

    default:
      CapricaError::logicalFatal("Unknown PapyrusTypeKind!");
  }
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
      return src.type != PapyrusType::Kind::None;
    case PapyrusType::Kind::None:
    case PapyrusType::Kind::Int:
    case PapyrusType::Kind::Array:
    case PapyrusType::Kind::Unresolved:
    case PapyrusType::Kind::ResolvedStruct:
      return false;
    default:
      CapricaError::logicalFatal("Unknown PapyrusTypeKind!");
  }
}

bool PapyrusResolutionContext::canImplicitlyCoerceExpression(expressions::PapyrusExpression* expr, const PapyrusType& target, bool& needsCast) {
  needsCast = true;
  bool canCast = canImplicitlyCoerce(expr->resultType(), target);
  switch (target.type) {
    case PapyrusType::Kind::Bool:
    case PapyrusType::Kind::Int:
    case PapyrusType::Kind::Float:
    case PapyrusType::Kind::String:
    case PapyrusType::Kind::Unresolved:
      break;
    case PapyrusType::Kind::Var:
      // Implicit conversion from None->Var is allowed, but only for a literal None.
      if (expr->resultType().type == PapyrusType::Kind::None && expr->is<expressions::PapyrusLiteralExpression>()) {
        canCast = true;
        needsCast = false;
      }
      break;
    case PapyrusType::Kind::Array:
      // Implicit conversion from None->Array is allowed, but only for a literal None.
      if (expr->resultType().type == PapyrusType::Kind::None && expr->is<expressions::PapyrusLiteralExpression>()) {
        canCast = true;
        needsCast = false;
      }
      break;
    case PapyrusType::Kind::ResolvedObject:
      // Implicit conversion from None->Object is allowed, but only for a literal None.
      if (expr->resultType().type == PapyrusType::Kind::None && expr->is<expressions::PapyrusLiteralExpression>()) {
        canCast = true;
        needsCast = false;
      }
      break;
    case PapyrusType::Kind::ResolvedStruct:
      // Implicit conversion from None->Struct is allowed, but only for a literal None.
      if (expr->resultType().type == PapyrusType::Kind::None && expr->is<expressions::PapyrusLiteralExpression>()) {
        canCast = true;
        needsCast = false;
      }
      break;
    default:
      CapricaError::logicalFatal("Unknown PapyrusTypeKind!");
  }
  return canCast;
}

expressions::PapyrusExpression* PapyrusResolutionContext::coerceExpression(expressions::PapyrusExpression* expr, const PapyrusType& target) {
  if (expr->resultType() != target) {
    bool needsCast = true;
    bool canCast = canImplicitlyCoerceExpression(expr, target, needsCast);

    if (canCast && CapricaConfig::enableOptimizations && expr->resultType().type == PapyrusType::Kind::Int && target.type == PapyrusType::Kind::Float) {
      if (auto le = expr->as<expressions::PapyrusLiteralExpression>()) {
        le->value.f = (float)le->value.i;
        le->value.type = PapyrusValueType::Float;
        return expr;
      }
    }

    if (!canCast) {
      CapricaError::error(expr->location, "No implicit conversion from '%s' to '%s' exists!", expr->resultType().prettyString().c_str(), target.prettyString().c_str());
      return expr;
    }
    if (!needsCast)
      return expr;
    auto ce = new expressions::PapyrusCastExpression(expr->location, target);
    ce->innerExpression = expr;
    return ce;
  }
  return expr;
}

PapyrusType PapyrusResolutionContext::resolveType(PapyrusType tp) {
  if (tp.type != PapyrusType::Kind::Unresolved) {
    if (tp.type == PapyrusType::Kind::Array)
      return PapyrusType::Array(tp.location, std::make_shared<PapyrusType>(resolveType(tp.getElementType())));
    return tp;
  }

  if (isPexResolution || CapricaConfig::enableDecompiledStructNameRefs) {
    auto pos = tp.name.find_first_of('#');
    if (pos != std::string::npos) {
      auto scName = tp.name.substr(0, pos);
      auto strucName = tp.name.substr(pos + 1);
      auto sc = loadScript(scName);
      if (!sc)
        CapricaError::fatal(tp.location, "Unable to find script '%s' referenced by '%s'!", scName.c_str(), tp.name.c_str());

      for (auto obj : sc->objects) {
        for (auto struc : obj->structs) {
          if (!_stricmp(struc->name.c_str(), strucName.c_str())) {
            tp.type = PapyrusType::Kind::ResolvedStruct;
            tp.resolvedStruct = struc;
            return tp;
          }
        }
      }

      CapricaError::fatal(tp.location, "Unable to resolve a struct named '%s' in script '%s'!", strucName.c_str(), scName.c_str());
    }
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

  for (auto sc : importedScripts) {
    for (auto obj : sc->objects) {
      for (auto struc : obj->structs) {
        if (!_stricmp(struc->name.c_str(), tp.name.c_str())) {
          tp.type = PapyrusType::Kind::ResolvedStruct;
          tp.resolvedStruct = struc;
          return tp;
        }
      }
    }
  }

  auto sc = loadScript(tp.name);
  if (sc != nullptr) {
    for (auto obj : sc->objects) {
      if (!_stricmp(obj->name.c_str(), tp.name.c_str())) {
        tp.type = PapyrusType::Kind::ResolvedObject;
        tp.resolvedObject = obj;
        return tp;
      }
    }
  }
  
  CapricaError::fatal(tp.location, "Unable to resolve type '%s'!", tp.name.c_str());
}

void PapyrusResolutionContext::addIdentifier(const PapyrusIdentifier& ident) {
  // TODO: Deal with checking identifiers on the parent class.
  for (auto is : identifierStack) {
    if (is.count(ident.name)) {
      CapricaError::error(ident.location, "Attempted to redefined '%s' which was already defined in a parent scope!", ident.name.c_str());
      return;
    }
  }
  identifierStack.back().insert({ ident.name, ident });
}

PapyrusIdentifier PapyrusResolutionContext::resolveIdentifier(const PapyrusIdentifier& ident) const {
  auto id = tryResolveIdentifier(ident);
  if (id.type == PapyrusIdentifierType::Unresolved)
    CapricaError::fatal(ident.location, "Unresolved identifier '%s'!", ident.name.c_str());
  return id;
}

PapyrusIdentifier PapyrusResolutionContext::tryResolveIdentifier(const PapyrusIdentifier& ident) const {
  if (ident.type != PapyrusIdentifierType::Unresolved)
    return ident;

  for (auto& stack : boost::adaptors::reverse(identifierStack)) {
    auto f = stack.find(ident.name);
    if (f != stack.end()) {
      return f->second;
    }
  }

  if (object->parentClass.type != PapyrusType::Kind::None) {
    if (object->parentClass.type != PapyrusType::Kind::ResolvedObject)
      CapricaError::logicalFatal("Something is wrong here, this should already have been resolved!");
    return tryResolveMemberIdentifier(object->parentClass, ident);
  }

  return ident;
}

PapyrusIdentifier PapyrusResolutionContext::resolveMemberIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const {
  auto id = tryResolveMemberIdentifier(baseType, ident);
  if (id.type == PapyrusIdentifierType::Unresolved)
    CapricaError::fatal(ident.location, "Unresolved identifier '%s'!", ident.name.c_str());
  return id;
}

PapyrusIdentifier PapyrusResolutionContext::tryResolveMemberIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const {
  if (ident.type != PapyrusIdentifierType::Unresolved)
    return ident;

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

    if (baseType.resolvedObject->parentClass.type != PapyrusType::Kind::None) {
      if (baseType.resolvedObject->parentClass.type != PapyrusType::Kind::ResolvedObject)
        CapricaError::logicalFatal("Something is wrong here, this should already have been resolved!");

      return tryResolveMemberIdentifier(baseType.resolvedObject->parentClass, ident);
    }
  }

  return ident;
}

PapyrusIdentifier PapyrusResolutionContext::resolveFunctionIdentifier(const PapyrusType& baseType, const PapyrusIdentifier& ident) const {
  if (ident.type != PapyrusIdentifierType::Unresolved)
    return ident;

  if (baseType.type == PapyrusType::Kind::None) {
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

    for (auto sc : importedScripts) {
      for (auto obj : sc->objects) {
        for (auto stat : obj->states) {
          for (auto func : stat->functions) {
            if (func->isGlobal && !_stricmp(func->name.c_str(), ident.name.c_str())) {
              PapyrusIdentifier id = ident;
              id.type = PapyrusIdentifierType::Function;
              id.func = func;
              return id;
            }
          }
        }
      }
    }
  } else if (baseType.type == PapyrusType::Kind::Array) {
    auto fk = PapyrusBuiltinArrayFunctionKind::Unknown;
    if (!_stricmp(ident.name.c_str(), "find")) {
      if (baseType.getElementType().type == PapyrusType::Kind::ResolvedStruct)
        fk = PapyrusBuiltinArrayFunctionKind::FindStruct;
      else
        fk = PapyrusBuiltinArrayFunctionKind::Find;
    } else if (!_stricmp(ident.name.c_str(), "rfind")) {
      if (baseType.getElementType().type == PapyrusType::Kind::ResolvedStruct)
        fk = PapyrusBuiltinArrayFunctionKind::RFindStruct;
      else
        fk = PapyrusBuiltinArrayFunctionKind::RFind;
    } else if (!_stricmp(ident.name.c_str(), "add")) {
      fk = PapyrusBuiltinArrayFunctionKind::Add;
    } else if (!_stricmp(ident.name.c_str(), "clear")) {
      fk = PapyrusBuiltinArrayFunctionKind::Clear;
    } else if (!_stricmp(ident.name.c_str(), "insert")) {
      fk = PapyrusBuiltinArrayFunctionKind::Insert;
    } else if (!_stricmp(ident.name.c_str(), "remove")) {
      fk = PapyrusBuiltinArrayFunctionKind::Remove;
    } else if (!_stricmp(ident.name.c_str(), "removelast")) {
      fk = PapyrusBuiltinArrayFunctionKind::RemoveLast;
    } else {
      CapricaError::fatal(ident.location, "Unknown function '%s' called on an array expression!", ident.name.c_str());
    }
    return PapyrusIdentifier::ArrayFunction(baseType.location, fk, baseType.getElementType());
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

    if (baseType.resolvedObject->parentClass.type != PapyrusType::Kind::None) {
      if (baseType.resolvedObject->parentClass.type != PapyrusType::Kind::ResolvedObject)
        CapricaError::logicalFatal("Something is wrong here, this should already have been resolved!");
      
      return resolveFunctionIdentifier(baseType.resolvedObject->parentClass, ident);
    }
  }

  CapricaError::fatal(ident.location, "Unresolved function name '%s'!", ident.name.c_str());
}

}}
