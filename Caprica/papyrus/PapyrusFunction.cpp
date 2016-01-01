#include <papyrus/PapyrusFunction.h>

namespace caprica { namespace papyrus {

pex::PexFunction* PapyrusFunction::buildPex(pex::PexFile* file,
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
  // A couple of compiler-generated functions.
  if (name == "GetState") {
    bldr << pex::op::ret{ pex::PexValue::Identifier(file->getString("::State")) };
  } else if (name == "GotoState") {
    auto noneVar = bldr.getNoneLocal(file);
    auto soldState = bldr.allocateLocal(file, "soldState", PapyrusType::String());
    bldr << pex::op::assign{ soldState, pex::PexValue::Identifier(file->getString("::State")) };
    bldr << pex::op::callmethod{
      file->getString("OnEndState"),
      pex::PexValue::Identifier(file->getString("self")),
      noneVar,
      {
        pex::PexValue::Integer(1),
        pex::PexValue::Identifier(file->getString("asNewState"))
      }
    };
    bldr << pex::op::assign{ pex::PexValue::Identifier(file->getString("::State")), pex::PexValue::Identifier(file->getString("asNewState")) };
    bldr << pex::op::callmethod{
      file->getString("OnBeginState"),
      pex::PexValue::Identifier(file->getString("self")),
      noneVar,
      {
        pex::PexValue::Integer(1),
        soldState
      }
    };
  } else {
    for (auto s : statements)
      s->buildPex(file, bldr);
  }
  bldr.populateFunction(func, fDebInfo);


  if (file->debugInfo)
    file->debugInfo->functions.push_back(fDebInfo);
  else
    delete fDebInfo;

  return func;
}

}}
