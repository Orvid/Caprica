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
  PapyrusUserFlags userFlags{ PapyrusUserFlags::None };
  PapyrusType parentClass;
  PapyrusState* autoState{ nullptr };
  
  CapricaFileLocation location;

  std::vector<std::pair<CapricaFileLocation, std::string>> imports{ };
  std::vector<PapyrusStruct*> structs{ };
  std::vector<PapyrusVariable*> variables{ };
  std::vector<PapyrusPropertyGroup*> propertyGroups{ };
  std::vector<PapyrusState*> states{ };

  PapyrusObject(const CapricaFileLocation& loc, const PapyrusType& baseTp) : location(loc), parentClass(baseTp) { }
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
      rootPropertyGroup = new PapyrusPropertyGroup();
      propertyGroups.push_back(rootPropertyGroup);
    }
    return rootPropertyGroup;
  }

  PapyrusState* getRootState() {
    if (!rootState) {
      rootState = new PapyrusState();
      states.push_back(rootState);
    }
    return rootState;
  }

  void buildPex(pex::PexFile* file) const {
    auto obj = new pex::PexObject();
    obj->name = file->getString(name);
    obj->parentClassName = parentClass.buildPex(file);
    obj->documentationString = file->getString(documentationString);
    obj->isConst = isConst;
    if (autoState)
      obj->autoStateName = file->getString(autoState->name);
    else
      obj->autoStateName = file->getString("");
    obj->userFlags = buildPexUserFlags(file, userFlags);

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
    ctx->pushIdentifierScope();
    for (auto i : imports)
      ctx->addImport(i.first, i.second);
    for (auto s : structs)
      s->semantic(ctx);
    for (auto v : variables)
      v->semantic(ctx);
    for (auto g : propertyGroups)
      g->semantic(ctx);
    for (auto s : states)
      s->semantic(ctx);

    if (!ctx->isExternalResolution) {
      // The first pass resolves the types on the public API,
      // property types, return types, and parameter types.
      // This second pass resolves the actual identifiers and
      // types in the bodies of functions.
      for (auto g : propertyGroups)
        g->semantic2(ctx);
      for (auto s : states)
        s->semantic2(ctx);
    }
    ctx->popIdentifierScope();
    ctx->object = nullptr;
  }

private:
  PapyrusState* rootState{ nullptr };
  PapyrusPropertyGroup* rootPropertyGroup{ nullptr };
};

}}
