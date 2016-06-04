#include <papyrus/PapyrusObject.h>

namespace caprica { namespace papyrus {

PapyrusPropertyGroup* PapyrusObject::getRootPropertyGroup() {
  if (!rootPropertyGroup) {
    rootPropertyGroup = new PapyrusPropertyGroup(location);
    propertyGroups.push_back(rootPropertyGroup);
  }
  return rootPropertyGroup;
}

const PapyrusObject* PapyrusObject::tryGetParentClass() const {
  if (parentClass.type != PapyrusType::Kind::None) {
    if (parentClass.type != PapyrusType::Kind::ResolvedObject)
      CapricaReportingContext::logicalFatal("Something is wrong here, this should already have been resolved!");
    return parentClass.resolvedObject;
  }
  return nullptr;
}

void PapyrusObject::buildPex(CapricaReportingContext& repCtx, pex::PexFile* file) const {
  auto obj = new pex::PexObject();
  obj->name = file->getString(name);
  if (auto parClass = tryGetParentClass())
    obj->parentClassName = file->getString(parClass->name);
  else
    obj->parentClassName = file->getString("");
  obj->documentationString = file->getString(documentationString);
  obj->isConst = isConst();
  if (autoState)
    obj->autoStateName = file->getString(autoState->name);
  else
    obj->autoStateName = file->getString("");
  obj->userFlags = userFlags.buildPex(file);

  for (auto s : structs)
    s->buildPex(repCtx, file, obj);
  for (auto v : variables)
    v->buildPex(repCtx, file, obj);
  for (auto g : propertyGroups)
    g->buildPex(repCtx, file, obj);

  size_t namedStateCount = 0;
  for (auto s : states) {
    if (s->name != "")
      namedStateCount++;
    s->buildPex(repCtx, file, obj);
  }

  size_t initialValueCount = 0;
  for (auto v : obj->variables) {
    if (v->defaultValue.type != pex::PexValueType::None)
      initialValueCount++;
  }

  EngineLimits::checkLimit(repCtx, location, EngineLimits::Type::PexObject_InitialValueCount, initialValueCount);
  EngineLimits::checkLimit(repCtx, location, EngineLimits::Type::PexObject_NamedStateCount, namedStateCount);
  EngineLimits::checkLimit(repCtx, location, EngineLimits::Type::PexObject_PropertyCount, obj->properties.size());
  EngineLimits::checkLimit(repCtx, location, EngineLimits::Type::PexObject_VariableCount, obj->variables.size());

  file->objects.push_back(obj);
}

void PapyrusObject::semantic(PapyrusResolutionContext* ctx) {
  resolutionState = PapyrusResoultionState::SemanticInProgress;
  ctx->object = this;
  for (auto i : imports)
    ctx->addImport(i.first, i.second);
  for (auto s : structs)
    s->semantic(ctx);
  for (auto g : propertyGroups)
    g->semantic(ctx);
  for (auto s : states)
    s->semantic(ctx);
  ctx->clearImports();
  ctx->object = nullptr;
  resolutionState = PapyrusResoultionState::SemanticCompleted;
}

void PapyrusObject::semantic2(PapyrusResolutionContext* ctx) {
  resolutionState = PapyrusResoultionState::Semantic2InProgress;
  ctx->object = this;
  for (auto i : imports)
    ctx->addImport(i.first, i.second);
  ctx->ensureNamesAreUnique(structs, "struct");
  ctx->ensureNamesAreUnique(variables, "variable");
  ctx->ensureNamesAreUnique(propertyGroups, "property group");
  ctx->ensureNamesAreUnique(states, "state");
  ctx->ensureNamesAreUnique(customEvents, "custom event");

  caseless_unordered_identifier_map<std::pair<bool, std::string>> identMap{ };
  checkForInheritedIdentifierConflicts(ctx->reportingContext, identMap, false);

  // The first pass resolves the types on the public API,
  // property types, return types, and parameter types.
  // This second pass resolves the actual identifiers and
  // types in the bodies of functions.
  for (auto v : variables)
    v->semantic2(ctx);
  for (auto g : propertyGroups)
    g->semantic2(ctx);
  for (auto s : states)
    s->semantic2(ctx);
  for (auto c : customEvents)
    c->semantic2(ctx);

  for (auto v : variables) {
    if (!v->referenceState.isRead) {
      if (!v->referenceState.isInitialized) {
        if (v->referenceState.isWritten)
          ctx->reportingContext.warning_W4006_Script_Variable_Only_Written(v->location, v->name.c_str());
        else
          ctx->reportingContext.warning_W4004_Unreferenced_Script_Variable(v->location, v->name.c_str());
      } else {
        ctx->reportingContext.warning_W4007_Script_Variable_Initialized_Never_Used(v->location, v->name.c_str());
      }
    } else if (!v->referenceState.isInitialized && !v->referenceState.isWritten) {
      ctx->reportingContext.warning_W4005_Unwritten_Script_Variable(v->location, v->name.c_str());
    }
  }
  ctx->clearImports();
  ctx->object = nullptr;
  resolutionState = PapyrusResoultionState::Semantic2Completed;
}

void PapyrusObject::checkForInheritedIdentifierConflicts(CapricaReportingContext& repCtx, caseless_unordered_identifier_map<std::pair<bool, std::string>>& identMap, bool checkInheritedOnly) const {
  if (auto parent = tryGetParentClass())
    parent->checkForInheritedIdentifierConflicts(repCtx, identMap, true);

  const auto doError = [](CapricaReportingContext& repCtx, CapricaFileLocation loc, bool isParent, const std::string& otherType, const std::string& identName) {
    if (isParent)
      repCtx.error(loc, "A parent object already defines a %s named '%s'.", otherType.c_str(), identName.c_str());
    else
      repCtx.error(loc, "A %s named '%s' was already defined in this object.", otherType.c_str(), identName.c_str());
  };

  for (auto pg : propertyGroups) {
    for (auto p : pg->properties) {
      auto f = identMap.find(p->name);
      if (f != identMap.end())
        doError(repCtx, p->location, f->second.first, f->second.second, p->name);
      else
        identMap.insert({ p->name, std::make_pair(checkInheritedOnly, "property") });
    }
  }

  for (auto s : structs) {
    auto f = identMap.find(s->name);
    if (f != identMap.end())
      doError(repCtx, s->location, f->second.first, f->second.second, s->name);
    else
      identMap.insert({ s->name, std::make_pair(checkInheritedOnly, "struct") });
  }

  /* Custom events are currently allowed to have the same names as properties -_-...
  for (auto e : customEvents) {
  auto f = identMap.find(e->name);
  if (f != identMap.end())
  doError(repCtx, e->location, f->second.first, f->second.second, e->name);
  else
  identMap.insert({ e->name, std::make_pair(checkInheritedOnly, "custom event") });
  }
  */

  if (!checkInheritedOnly) {
    for (auto v : variables) {
      auto f = identMap.find(v->name);
      if (f != identMap.end())
        doError(repCtx, v->location, f->second.first, f->second.second, v->name);
      else
        identMap.insert({ v->name, std::make_pair(checkInheritedOnly, "variable") });
    }
  }
}

}}
