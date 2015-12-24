#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusProperty.h>
#include <papyrus/PapyrusPropertyGroup.h>
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
  PapyrusType parentClass{ };
  PapyrusState* autoState{ nullptr };

  std::vector<PapyrusStruct*> structs{ };
  std::vector<PapyrusVariable*> variables{ };
  std::vector<PapyrusProperty*> properties{ };
  std::vector<PapyrusPropertyGroup*> propertyGroups{ };
  std::vector<PapyrusState*> states{ };

  PapyrusObject() = default;
  ~PapyrusObject() {
    for (auto s : structs)
      delete s;
    for (auto v : variables)
      delete v;
    for (auto p : properties)
      delete p;
    for (auto g : propertyGroups)
      delete g;
    for (auto s : states)
      delete s;
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
    obj->userFlags = userFlags;

    for (auto s : structs)
      s->buildPex(file, obj);
    for (auto v : variables)
      v->buildPex(file, obj);
    for (auto p : properties)
      p->buildPex(file, obj);
    for (auto g : propertyGroups)
      g->buildPex(file, obj);
    for (auto s : states)
      s->buildPex(file, obj);

    file->objects.push_back(obj);
  }
};

}}
