#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusFunctionParameter.h>
#include <papyrus/PapyrusLocalVariable.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexFile.h>
#include <pex/PexFunction.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexObject.h>
#include <pex/PexState.h>

namespace caprica { namespace papyrus {

struct PapyrusFunction final
{
  std::string name{ "" };
  std::string documentationComment{ "" };
  PapyrusType returnType{ };
  PapyrusUserFlags userFlags{ PapyrusUserFlags::None };
  bool isGlobal{ false };
  bool isNative{ false };
  std::vector<PapyrusFunctionParameter*> parameters{ };
  std::vector<PapyrusLocalVariable*> locals{ };
  std::vector<statements::PapyrusStatement*> statements{ };

  PapyrusFunction() = default;
  ~PapyrusFunction() {
    for (auto p : parameters)
      delete p;
    for (auto l : locals)
      delete l;
    for (auto s : statements)
      delete s;
  }

  void buildPex(pex::PexFile* file, pex::PexObject* obj, pex::PexState* state) const {
    auto func = new pex::PexFunction();
    func->name = file->getString(name);
    func->documenationString = file->getString(documentationComment);
    func->returnTypeName = returnType.buildPex(file);
    func->userFlags = userFlags;
    func->isGlobal = isGlobal;
    func->isNative = isNative;
    for (auto p : parameters)
      p->buildPex(file, obj, state, func);

    auto local = new pex::PexLocalVariable();
    local->name = file->getString("test");
    local->type = file->getString("Int");
    func->locals.push_back(local);

    pex::PexFunctionBuilder funcBuilder;
    funcBuilder << pex::op::nop{ };
    funcBuilder << pex::op::iadd(local, local, pex::PexValue::Integer(1));
    funcBuilder.populateFunction(func);

    state->functions.push_back(func);
  }
};

}}
