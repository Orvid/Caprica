#include <papyrus/PapyrusObject.h>

#include <papyrus/PapyrusCompilationContext.h>

namespace caprica { namespace papyrus {

PapyrusObject* PapyrusObject::awaitSemantic() const {
  return compilationNode->awaitSemantic();
}

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
    return parentClass.resolved.obj;
  }
  return nullptr;
}

void PapyrusObject::buildPex(CapricaReportingContext& repCtx, pex::PexFile* file) const {
  auto obj = file->alloc->make<pex::PexObject>();
  obj->name = file->getString(name);
  if (auto parClass = tryGetParentClass()) {
    if (file->gameID != GameID::Skyrim || parClass->name != "__ScriptObject")
      obj->parentClassName = file->getString(parClass->name);
    else {
      obj->parentClassName = file->getString("");
    }
  } else {
    obj->parentClassName = file->getString("");
  }
  obj->documentationString = file->getString(documentationString);
  obj->isConst = isConst();
  if (autoState)
    obj->autoStateName = file->getString(autoState->name);
  else
    obj->autoStateName = file->getString("");
  obj->userFlags = userFlags.buildPex(file);

  if (file->gameID > GameID::Skyrim){
    for (auto s : structs)
      s->buildPex(repCtx, file, obj);
  }

  for (auto v : variables)
    v->buildPex(repCtx, file, obj);

  if (file->gameID == GameID::Starfield) {
    for (auto g : guards)
      g->buildPex(repCtx, file, obj);
  }

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
  // TODO: Make this configurable for Starfield
  EngineLimits::checkLimit(repCtx, location, EngineLimits::Type::PexObject_GuardCount, obj->guards.size());

  file->objects.push_back(obj);
}

void PapyrusObject::semantic(PapyrusResolutionContext* ctx) {
  resolutionState = PapyrusResoultionState::SemanticInProgress;
  if (auto c = this->tryGetParentClass())
    c->awaitSemantic();
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
  ctx->ensureNamesAreUnique(guards, "guard");
  ctx->ensureNamesAreUnique(propertyGroups, "property group");
  ctx->ensureNamesAreUnique(states, "state");
  ctx->ensureNamesAreUnique(customEvents, "custom event");

  caseless_unordered_identifier_ref_map<std::pair<bool, const char*>> identMap{ };
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
  // TODO: Make this configurable for Starfield
  for (auto g : guards)
    g->semantic2(ctx);

  for (auto v : variables) {
    if (!v->referenceState.isRead) {
      if (!v->referenceState.isInitialized) {
        if (v->referenceState.isWritten)
          ctx->reportingContext.warning_W4006_Script_Variable_Only_Written(v->location, v->name.to_string().c_str());
        else
          ctx->reportingContext.warning_W4004_Unreferenced_Script_Variable(v->location, v->name.to_string().c_str());
      } else {
        ctx->reportingContext.warning_W4007_Script_Variable_Initialized_Never_Used(v->location, v->name.to_string().c_str());
      }
    } else if (!v->referenceState.isInitialized && !v->referenceState.isWritten) {
      ctx->reportingContext.warning_W4005_Unwritten_Script_Variable(v->location, v->name.to_string().c_str());
    }
  }
  ctx->clearImports();
  ctx->object = nullptr;
  resolutionState = PapyrusResoultionState::Semantic2Completed;
}

void PapyrusObject::checkForInheritedIdentifierConflicts(CapricaReportingContext& repCtx, caseless_unordered_identifier_ref_map<std::pair<bool, const char*>>& identMap, bool checkInheritedOnly) const {
  if (auto parent = tryGetParentClass())
    parent->checkForInheritedIdentifierConflicts(repCtx, identMap, true);

  const auto doError = [](CapricaReportingContext& repCtx, CapricaFileLocation loc, bool isParent, const char* otherType, const identifier_ref& identName) {
    if (isParent)
      repCtx.error(loc, "A parent object already defines a %s named '%s'.", otherType, identName.to_string().c_str());
    else
      repCtx.error(loc, "A %s named '%s' was already defined in this object.", otherType, identName.to_string().c_str());
  };

  for (auto pg : propertyGroups) {
    for (auto p : pg->properties) {
      auto f = identMap.find(p->name);
      if (f != identMap.end())
        doError(repCtx, p->location, f->second.first, f->second.second, p->name);
      else
        identMap.emplace(p->name, std::make_pair(checkInheritedOnly, "property"));
    }
  }

  for (auto s : structs) {
    auto f = identMap.find(s->name);
    if (f != identMap.end())
      doError(repCtx, s->location, f->second.first, f->second.second, s->name);
    else
      identMap.emplace(s->name, std::make_pair(checkInheritedOnly, "struct"));
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
      if (f != identMap.end()) {
        if (conf::Papyrus::game == GameID::Skyrim && conf::Skyrim::skyrimAllowObjectVariableShadowingParentProperty &&
              _stricmp(f->second.second, "property") == 0){
          repCtx.warning_W7001_Skyrim_Child_Variable_Shadows_Parent_Property(
                  v->location,
                  v->name.to_string().c_str(),
                  name.to_string().c_str(),
                  parentClass.prettyString().c_str());
        } else {
          doError(repCtx, v->location, f->second.first, f->second.second, v->name);
        }
      }
      else
        identMap.emplace(v->name, std::make_pair(checkInheritedOnly, "variable"));
    }
  }
  // TODO: Make this configurable for Starfield
  // TODO: Starfield: Verify that guards in child classes are not allowed to have the same name as guards in parent classes.
  // guards
  for (auto g : guards){
    auto f = identMap.find(g->name);
    if (f != identMap.end())
      doError(repCtx, g->location, f->second.first, f->second.second, g->name);
    else
      identMap.emplace(g->name, std::make_pair(checkInheritedOnly, "guard"));
  }
}

}}
