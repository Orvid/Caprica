#include <papyrus/PapyrusFunction.h>

#include <unordered_set>

#include <common/EngineLimits.h>

#include <papyrus/PapyrusCFG.h>
#include <papyrus/PapyrusObject.h>
#include <papyrus/PapyrusState.h>
#include <papyrus/statements/PapyrusDeclareStatement.h>
#include <papyrus/statements/PapyrusStatementVisitor.h>

namespace caprica { namespace papyrus {

bool PapyrusFunction::isBetaOnly() const {
  return userFlags.isBetaOnly || parentObject->isBetaOnly();
}
bool PapyrusFunction::isDebugOnly() const {
  return userFlags.isDebugOnly || parentObject->isDebugOnly();
}

pex::PexFunction* PapyrusFunction::buildPex(CapricaReportingContext& repCtx,
                                            pex::PexFile* file,
                                            pex::PexObject* obj,
                                            pex::PexState* state,
                                            pex::PexString propName) const {
  auto func = file->alloc->make<pex::PexFunction>();
  auto fDebInfo = file->alloc->make<pex::PexDebugFunctionInfo>();
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

  pex::PexFunctionBuilder bldr { repCtx, location, file };
  for (auto s : statements)
    s->buildPex(file, bldr);
  bldr.populateFunction(func, fDebInfo);

  if (file->debugInfo)
    file->debugInfo->functions.push_back(fDebInfo);

  EngineLimits::checkLimit(repCtx,
                           location,
                           EngineLimits::Type::PexFunction_ParameterCount,
                           func->parameters.size(),
                           name);
  return func;
}

void PapyrusFunction::semantic(PapyrusResolutionContext* ctx) {
  returnType = ctx->resolveType(returnType, true);
  if (isBetaOnly())
    returnType.poison(PapyrusType::PoisonKind::Beta);
  if (isDebugOnly())
    returnType.poison(PapyrusType::PoisonKind::Debug);

  for (auto p : parameters)
    p->semantic(ctx);
}

void PapyrusFunction::semantic2(PapyrusResolutionContext* ctx) {
  if (isGlobal() && ctx->state && ctx->state->name != "")
    ctx->reportingContext.error(location, "Global functions are only allowed in the empty state.");
  if (isNative() && !ctx->object->isNative())
    ctx->reportingContext.error(location, "You can only define Native functions in a script marked Native.");

  ctx->ensureNamesAreUnique(parameters, "parameter");

  ctx->function = this;
  ctx->pushLocalVariableScope();
  // skyrim first pass
  if (conf::Papyrus::game == GameID::Skyrim)
    for (auto s : statements)
      s->semantic_skyrim_first_pass(ctx);
  for (auto s : statements)
    s->semantic(ctx);
  ctx->popLocalVariableScope();
  ctx->function = nullptr;

  // Don't build the CFG for the special functions.
  if (!isNative()) {
    PapyrusCFG cfg { ctx->reportingContext };
    cfg.processStatements(statements);

    if (returnType.type != PapyrusType::Kind::None) {
      auto curNode = cfg.root;
      while (curNode != nullptr) {
        if (curNode->edgeType == PapyrusControlFlowNodeEdgeType::Children ||
            curNode->edgeType == PapyrusControlFlowNodeEdgeType::Return) {
          break;
        }
        curNode = curNode->nextSibling;
      }

      if (curNode == nullptr)
        ctx->reportingContext.warning_W1000_Strict_Not_All_Control_Paths_Return(location, name.to_string().c_str());
    }

    if (conf::Debug::debugControlFlowGraph) {
      // std::cout << "CFG for " << name << ":" << std::endl;
      cfg.dumpGraph();
    }
  }

  // We need to be able to distinguish between locals with the
  // same name defined in different scopes, so we have to mangle
  // the ones that are the same.
  struct CheckLocalNamesStatementVisitor final : statements::PapyrusSelectiveStatementVisitor {
    PapyrusResolutionContext* ctx;
    caseless_unordered_identifier_ref_set allLocalNames {};
    CheckLocalNamesStatementVisitor(PapyrusResolutionContext* oCtx) : ctx(oCtx) { }

    virtual void visit(statements::PapyrusDeclareStatement* s) override {
      int i = 0;
      auto baseName = s->name;
      while (allLocalNames.count(s->name))
        s->name = ctx->allocator->allocateIdentifier("::mangled_" + baseName.to_string() + "_" + std::to_string(i++));
      allLocalNames.insert(s->name);
    }
  } visitor(ctx);

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
  for (auto pbA = parameters.begin(),
            pbB = other->parameters.begin(),
            peA = parameters.end(),
            peB = other->parameters.end();
       pbA != peA && pbB != peB;
       ++pbA, ++pbB) {
    if (pbA->type != pbB->type)
      return false;
  }
  return true;
}

std::string PapyrusFunction::prettySignature() const {
  std::string str;
  str += returnType.prettyString();
  str += "(";
  for (auto p : parameters) {
    str += p->type.prettyString();
    if (p->index + 1 < parameters.size())
      str += ", ";
  }
  str += ")";

  return str;
}

}}
