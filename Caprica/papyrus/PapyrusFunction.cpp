#include <papyrus/PapyrusFunction.h>

#include <sstream>
#include <unordered_set>

#include <papyrus/statements/PapyrusDeclareStatement.h>
#include <papyrus/statements/PapyrusStatementVisitor.h>

namespace caprica { namespace papyrus {

pex::PexFunction* PapyrusFunction::buildPex(pex::PexFile* file,
                                            pex::PexObject* obj,
                                            pex::PexState* state,
                                            pex::PexString propName) const {
  auto func = new pex::PexFunction();
  auto fDebInfo = new pex::PexDebugFunctionInfo();
  fDebInfo->objectName = obj->name;
  switch (functionType) {
    case PapyrusFunctionType::Function:
    case PapyrusFunctionType::Event:
      fDebInfo->functionType = pex::PexDebugFunctionType::Normal;
      break;
    case PapyrusFunctionType::Getter:
      fDebInfo->functionType = pex::PexDebugFunctionType::Getter;
      break;
    case PapyrusFunctionType::Setter:
      fDebInfo->functionType = pex::PexDebugFunctionType::Setter;
      break;
    default:
      CapricaError::logicalFatal("Unknown PapyrusFunctionType!");
  }
  if (state) {
    assert(fDebInfo->functionType == pex::PexDebugFunctionType::Normal);
    fDebInfo->stateName = state->name;
    fDebInfo->functionName = file->getString(name);
    func->name = file->getString(name);
  } else {
    fDebInfo->stateName = file->getString("");
    fDebInfo->functionName = propName;
  }

  func->documentationString = file->getString(documentationComment);
  func->returnTypeName = returnType.buildPex(file);
  func->userFlags = buildPexUserFlags(file, userFlags);
  func->isGlobal = isGlobal;
  func->isNative = isNative;
  for (auto p : parameters)
    p->buildPex(file, obj, func);

  pex::PexFunctionBuilder bldr{ location };
  // A couple of compiler-generated functions.
  if (name == "GetState") {
    bldr << pex::op::ret{ pex::PexValue::Identifier(file->getString("::State")) };
  } else if (name == "GotoState") {
    auto noneVar = bldr.getNoneLocal(location, file);
    auto soldState = bldr.allocateLocal(file, "soldState", PapyrusType::String(location));
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

void PapyrusFunction::semantic(PapyrusResolutionContext* ctx) {
  returnType = ctx->resolveType(returnType);
  ctx->function = this;
  ctx->pushIdentifierScope();
  for (auto p : parameters)
    p->semantic(ctx);
  if (ctx->resolvingReferenceScript) {
    for (auto s : statements)
      delete s;
    statements.clear();
  }
  ctx->popIdentifierScope();
  ctx->function = nullptr;
}

void PapyrusFunction::semantic2(PapyrusResolutionContext* ctx) {
  returnType = ctx->resolveType(returnType);
  ctx->function = this;
  ctx->pushIdentifierScope();
  for (auto p : parameters)
    p->semantic2(ctx);
  for (auto s : statements)
    s->semantic(ctx);
  ctx->popIdentifierScope();
  ctx->function = nullptr;

  // We need to be able to distinguish between locals with the
  // same name defined in different scopes, so we have to mangle
  // the ones that are the same.
  struct CheckLocalNamesStatementVisitor final : statements::PapyrusStatementVisitor
  {
    std::unordered_set<std::string, CaselessStringHasher> allLocalNames{ };

    virtual void visit(statements::PapyrusDeclareStatement* s) {
      int i = 0;
      auto baseName = s->name;
      while (allLocalNames.count(s->name)) {
        std::ostringstream strm;
        strm << "::mangled_" << baseName << "_" << i;
        s->name = strm.str();
        i++;
      }
      allLocalNames.insert(s->name);
    }
  } visitor;

  for (auto s : statements)
    s->visit(visitor);
}

}}
