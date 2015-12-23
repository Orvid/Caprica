#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusObject.h>
#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>

namespace caprica { namespace papyrus {

struct PapyrusScript final
{
  std::string name{ "" };
  std::vector<PapyrusObject*> objects{ };

  PapyrusScript() = default;
  ~PapyrusScript() {
    for (auto obj : objects)
      delete obj;
  }

  pex::PexFile* buildPex() {
    auto pex = new pex::PexFile();
    auto obj = new pex::PexObject();
    obj->name = pex->getString("Test");
    obj->documentationString = pex->getString("");
    obj->parentClassName = pex->getString("Quest");
    obj->autoStateName = pex->getString("");
    pex->objects.push_back(obj);
    auto state = new pex::PexState();
    state->name = pex->getString("");
    obj->states.push_back(state);
    auto func = new pex::PexFunction();
    func->name = pex->getString("TestFunc");
    func->documenationString = pex->getString("");
    func->returnTypeName = pex->getString("none");

    auto local = new pex::PexLocalVariable();
    local->name = pex->getString("test");
    local->type = pex->getString("Int");

    pex::PexFunctionBuilder funcBuilder;
    funcBuilder << pex::op::nop{ };
    funcBuilder << pex::op::iadd(local, local, pex::PexValue::Integer(1));

    funcBuilder.populateFunction(func);
    state->functions.push_back(func);
    return pex;
  }
};

}}