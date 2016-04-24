#include <papyrus/PapyrusFunction.h>

#include <sstream>
#include <unordered_set>

#include <common/EngineLimits.h>

#include <papyrus/PapyrusCFG.h>
#include <papyrus/PapyrusState.h>
#include <papyrus/statements/PapyrusDeclareStatement.h>
#include <papyrus/statements/PapyrusStatementVisitor.h>

namespace caprica { namespace papyrus {

pex::PexFunction* PapyrusFunction::buildPex(CapricaReportingContext& repCtx, 
                                            pex::PexFile* file,
                                            pex::PexObject* obj,
                                            pex::PexState* state,
                                            pex::PexString propName) const {
  auto func = new pex::PexFunction();
  auto fDebInfo = new pex::PexDebugFunctionInfo();
  fDebInfo->objectName = obj->name;
  switch (functionType) {
    case PapyrusFunctionType::Function:
    case PapyrusFunctionType::Event:
    case PapyrusFunctionType::RemoteEvent:
      fDebInfo->functionType = pex::PexDebugFunctionType::Normal;
      break;
    case PapyrusFunctionType::Getter:
      fDebInfo->functionType = pex::PexDebugFunctionType::Getter;
      break;
    case PapyrusFunctionType::Setter:
      fDebInfo->functionType = pex::PexDebugFunctionType::Setter;
      break;
    case PapyrusFunctionType::Unknown:
      CapricaReportingContext::logicalFatal("Unknown PapyrusFunctionType!");
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
  func->userFlags = userFlags.buildPex(file);
  func->isGlobal = isGlobal();
  func->isNative = isNative();
  for (auto p : parameters)
    p->buildPex(file, obj, func);

  pex::PexFunctionBuilder bldr{ repCtx, location, file };
  for (auto s : statements)
    s->buildPex(file, bldr);
  bldr.populateFunction(func, fDebInfo);


  if (file->debugInfo)
    file->debugInfo->functions.push_back(fDebInfo);
  else
    delete fDebInfo;

  EngineLimits::checkLimit(repCtx, location, EngineLimits::Type::PexFunction_ParameterCount, func->parameters.size(), name.c_str());
  return func;
}

void PapyrusFunction::semantic(PapyrusResolutionContext* ctx) {
  returnType = ctx->resolveType(returnType);
  ctx->ensureNamesAreUnique(parameters, "parameter");
  for (auto p : parameters)
    p->semantic(ctx);

  // We don't care about the body in reference scripts.
  if (ctx->resolvingReferenceScript) {
    for (auto s : statements)
      delete s;
    statements.clear();
  }
}

void PapyrusFunction::semantic2(PapyrusResolutionContext* ctx) {
  if (isGlobal() && ctx->state && ctx->state->name != "")
    ctx->reportingContext.error(location, "Global functions are only allowed in the empty state.");

  ctx->function = this;
  ctx->pushLocalVariableScope();
  for (auto s : statements)
    s->semantic(ctx);
  ctx->popLocalVariableScope();
  ctx->function = nullptr;

  // Don't build the CFG for the special functions.
  if (!isNative() && name != "GetState" && name != "GotoState") {
    PapyrusCFG cfg{ ctx->reportingContext };
    cfg.processStatements(statements);

    if (returnType.type != PapyrusType::Kind::None) {
      auto curNode = cfg.root;
      while (curNode != nullptr) {
        if (curNode->edgeType == PapyrusControlFlowNodeEdgeType::Children || curNode->edgeType == PapyrusControlFlowNodeEdgeType::Return)
          break;
        curNode = curNode->nextSibling;
      }

      if (curNode == nullptr)
        ctx->reportingContext.error(location, "Not all control paths of '%s' return a value.", name.c_str());
    }

    if (CapricaConfig::debugControlFlowGraph) {
      std::cout << "CFG for " << name << ":" << std::endl;
      cfg.dumpGraph();
    }
  }

  // We need to be able to distinguish between locals with the
  // same name defined in different scopes, so we have to mangle
  // the ones that are the same.
  struct CheckLocalNamesStatementVisitor final : statements::PapyrusSelectiveStatementVisitor
  {
    std::unordered_set<std::string, CaselessStringHasher, CaselessStringEqual> allLocalNames{ };

    virtual void visit(statements::PapyrusDeclareStatement* s) override {
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

bool PapyrusFunction::hasSameSignature(const PapyrusFunction* other) const {
  if (!other)
    return false;
  if (parameters.size() != other->parameters.size())
    return false;
  if (returnType != other->returnType)
    return false;
  for (size_t i = 0; i < parameters.size(); i++) {
    if (parameters[i]->type != other->parameters[i]->type)
      return false;
  }
  return true;
}

std::string PapyrusFunction::prettySignature() const {
  std::string str;
  str += returnType.prettyString();
  str += "(";
  for (size_t i = 0; i < parameters.size(); i++) {
    str += parameters[i]->type.prettyString();
    if (i + 1 < parameters.size())
      str += ", ";
  }
  str += ")";

  return str;
}

}}
