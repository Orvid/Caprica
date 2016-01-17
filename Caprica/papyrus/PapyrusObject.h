#pragma once

#include <string>
#include <vector>

#include <common/CapricaFileLocation.h>
#include <papyrus/PapyrusProperty.h>
#include <papyrus/PapyrusPropertyGroup.h>
#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusState.h>
#include <papyrus/PapyrusStruct.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/PapyrusVariable.h>

#include <pex/PexFile.h>
#include <pex/PexObject.h>

namespace caprica { namespace papyrus {

struct PapyrusObject final
{
  std::string name{ "" };
  std::string documentationString{ "" };
  bool isConst{ false };
  PapyrusUserFlags userFlags{ };
  PapyrusType parentClass;
  PapyrusState* autoState{ nullptr };
  
  CapricaFileLocation location;

  std::vector<std::pair<CapricaFileLocation, std::string>> imports{ };
  std::vector<PapyrusStruct*> structs{ };
  std::vector<PapyrusVariable*> variables{ };
  std::vector<PapyrusPropertyGroup*> propertyGroups{ };
  std::vector<PapyrusState*> states{ };

  explicit PapyrusObject(const CapricaFileLocation& loc, const PapyrusType& baseTp) : location(loc), parentClass(baseTp) { }
  PapyrusObject(const PapyrusObject&) = delete;
  ~PapyrusObject() {
    for (auto s : structs)
      delete s;
    for (auto v : variables)
      delete v;
    for (auto g : propertyGroups)
      delete g;
    for (auto s : states)
      delete s;
  }

  PapyrusPropertyGroup* getRootPropertyGroup() {
    if (!rootPropertyGroup) {
      rootPropertyGroup = new PapyrusPropertyGroup(location);
      propertyGroups.push_back(rootPropertyGroup);
    }
    return rootPropertyGroup;
  }

  const PapyrusState* tryGetRootState() const { return rootState; }
  PapyrusState* getRootState() {
    if (!rootState) {
      rootState = new PapyrusState(location);
      states.push_back(rootState);
    }
    return rootState;
  }

  void buildPex(pex::PexFile* file) const {
    auto obj = new pex::PexObject();
    obj->name = file->getString(name);
    if (parentClass.type != PapyrusType::Kind::None) {
      if (parentClass.type != PapyrusType::Kind::ResolvedObject)
        CapricaError::logicalFatal("Something is wrong here, this should already have been resolved!");
      obj->parentClassName = file->getString(parentClass.resolvedObject->name);
    } else {
      obj->parentClassName = file->getString("");
    }
    obj->documentationString = file->getString(documentationString);
    obj->isConst = isConst;
    if (autoState)
      obj->autoStateName = file->getString(autoState->name);
    else
      obj->autoStateName = file->getString("");
    obj->userFlags = userFlags.buildPex(file);

    for (auto s : structs)
      s->buildPex(file, obj);
    for (auto v : variables)
      v->buildPex(file, obj);
    for (auto g : propertyGroups)
      g->buildPex(file, obj);
    for (auto s : states)
      s->buildPex(file, obj);

    file->objects.push_back(obj);
  }

  void semantic(PapyrusResolutionContext* ctx) {
    parentClass = ctx->resolveType(parentClass);
    ctx->object = this;
    for (auto i : imports)
      ctx->addImport(i.first, i.second);
    PapyrusResolutionContext::ensureNamesAreUnique(structs, "struct");
    for (auto s : structs)
      s->semantic(ctx);
    if (ctx->resolvingReferenceScript) {
      for (auto v : variables)
        delete v;
      variables.clear();
    } else {
      PapyrusResolutionContext::ensureNamesAreUnique(variables, "variable");
      for (auto v : variables)
        v->semantic(ctx);
    }
    PapyrusResolutionContext::ensureNamesAreUnique(propertyGroups, "property group");
    for (auto g : propertyGroups)
      g->semantic(ctx);
    PapyrusResolutionContext::ensureNamesAreUnique(states, "state");
    for (auto s : states)
      s->semantic(ctx);

    if (!ctx->resolvingReferenceScript) {
      std::map<std::string, std::pair<bool, std::string>, CaselessStringComparer> identMap{ };
      checkForInheritedIdentifierConflicts(identMap, false);

      // The first pass resolves the types on the public API,
      // property types, return types, and parameter types.
      // This second pass resolves the actual identifiers and
      // types in the bodies of functions.
      for (auto g : propertyGroups)
        g->semantic2(ctx);
      for (auto s : states)
        s->semantic2(ctx);
    }
    ctx->object = nullptr;
  }

private:
  PapyrusState* rootState{ nullptr };
  PapyrusPropertyGroup* rootPropertyGroup{ nullptr };

  void checkForInheritedIdentifierConflicts(std::map<std::string, std::pair<bool, std::string>, CaselessStringComparer>& identMap, bool checkInheritedOnly) const {
    if (parentClass.type != PapyrusType::Kind::None) {
      if (parentClass.type != PapyrusType::Kind::ResolvedObject)
        CapricaError::logicalFatal("Something is wrong here, this should already have been resolved!");
      parentClass.resolvedObject->checkForInheritedIdentifierConflicts(identMap, true);
    }

    const auto doError = [](const CapricaFileLocation& loc, bool isParent, const std::string& otherType, const std::string& identName) {
      if (isParent)
        CapricaError::error(loc, "A parent object already defines a %s named '%s'.", otherType.c_str(), identName.c_str());
      else
        CapricaError::error(loc, "A %s named '%s' was already defined in this object.", otherType.c_str(), identName.c_str());
    };

    for (auto pg : propertyGroups) {
      for (auto p : pg->properties) {
        auto f = identMap.find(p->name);
        if (f != identMap.end())
          doError(p->location, f->second.first, f->second.second, p->name);
        else
          identMap.insert({ p->name, std::make_pair(checkInheritedOnly, "property") });
      }
    }

    for (auto s : structs) {
      auto f = identMap.find(s->name);
      if (f != identMap.end())
        doError(s->location, f->second.first, f->second.second, s->name);
      else
        identMap.insert({ s->name, std::make_pair(checkInheritedOnly, "struct") });
    }

    if (!checkInheritedOnly) {
      for (auto v : variables) {
        auto f = identMap.find(v->name);
        if (f != identMap.end())
          doError(v->location, f->second.first, f->second.second, v->name);
        else
          identMap.insert({ v->name, std::make_pair(checkInheritedOnly, "variable") });
      }
    }
  }
};

}}
