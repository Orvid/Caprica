#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusFunctionParameter.h>
#include <papyrus/PapyrusLocalVariable.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/statements/PapyrusStatement.h>

#include <pex/PexDebugFunctionInfo.h>
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
  std::vector<statements::PapyrusStatement*> statements{ };

  PapyrusFunction() = default;
  ~PapyrusFunction() {
    for (auto p : parameters)
      delete p;
    for (auto s : statements)
      delete s;
  }

  pex::PexFunction* buildPex(pex::PexFile* file,
                             pex::PexObject* obj,
                             pex::PexState* state,
                             pex::PexDebugFunctionType funcType,
                             pex::PexString propName) const {
    auto func = new pex::PexFunction();
    auto fDebInfo = new pex::PexDebugFunctionInfo();
    fDebInfo->objectName = obj->name;
    fDebInfo->functionType = funcType;
    if (state) {
      assert(funcType == pex::PexDebugFunctionType::Normal);
      fDebInfo->stateName = state->name;
      fDebInfo->functionName = file->getString(name);
      func->name = file->getString(name);
    } else {
      fDebInfo->stateName = file->getString("");
      fDebInfo->functionName = propName;
    }

    func->documenationString = file->getString(documentationComment);
    func->returnTypeName = returnType.buildPex(file);
    func->userFlags = buildPexUserFlags(file, userFlags);
    func->isGlobal = isGlobal;
    func->isNative = isNative;
    for (auto p : parameters)
      p->buildPex(file, obj, func);

    pex::PexFunctionBuilder bldr;
    for (auto s : statements)
      s->buildPex(file, bldr);
    bldr.populateFunction(func, fDebInfo);

    if (file->debugInfo)
      file->debugInfo->functions.push_back(fDebInfo);
    else
      delete fDebInfo;

    return func;
  }
};

}}
