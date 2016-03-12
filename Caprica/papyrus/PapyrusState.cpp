#include <papyrus/PapyrusState.h>

#include <papyrus/PapyrusObject.h>

namespace caprica { namespace papyrus {

static PapyrusFunction* searchRootStateForFunction(const std::string& name, const PapyrusObject* obj) {
  auto rs = obj->tryGetRootState();
  if (rs != nullptr) {
    for (auto f : rs->functions) {
      if (!_stricmp(f->name.c_str(), name.c_str()))
        return f;
    }
  }

  if (obj->parentClass.type != PapyrusType::Kind::None) {
    if (obj->parentClass.type != PapyrusType::Kind::ResolvedObject)
      CapricaError::logicalFatal("Something is wrong here, this should already have been resolved!");
    return searchRootStateForFunction(name, obj->parentClass.resolvedObject);
  }

  return nullptr;
}

void PapyrusState::semantic(PapyrusResolutionContext* ctx) {
  ctx->state = this;
  PapyrusResolutionContext::ensureNamesAreUnique(functions, "function");
  for (auto f : functions)
    f->semantic(ctx);
  ctx->state = nullptr;
}

void PapyrusState::semantic2(PapyrusResolutionContext* ctx) {
  ctx->state = this;
  if (name != "") {
    for (auto f : functions) {
      auto baseFunc = searchRootStateForFunction(f->name, ctx->object);
      if (!baseFunc)
        CapricaError::error(f->location, "Function '%s' cannot be defined in state '%s' without also being defined in the empty state!", f->name.c_str(), name.c_str());
      else if (!baseFunc->hasSameSignature(f))
        CapricaError::error(f->location, "The signature of the '%s' function (%s) in the '%s' state doesn't match the signature in the root state. The expected signature is '%s'.", f->name.c_str(), f->prettySignature().c_str(), name.c_str(), baseFunc->prettySignature().c_str());
    }
  }

  if (ctx->object->parentClass.type != PapyrusType::Kind::None) {
    if (ctx->object->parentClass.type != PapyrusType::Kind::ResolvedObject)
      CapricaError::logicalFatal("Something is wrong here, this should already have been resolved!");
    for (auto f : functions) {
      auto baseFunc = searchRootStateForFunction(f->name, ctx->object->parentClass.resolvedObject);
      if (baseFunc && !baseFunc->hasSameSignature(f))
        CapricaError::error(f->location, "The signature of the '%s' function (%s) doesn't match the signature in the parent class '%s'. The expected signature is '%s'.", f->name.c_str(), f->prettySignature().c_str(), baseFunc->parentObject->name.c_str(), baseFunc->prettySignature().c_str());
    }
  }

  for (auto f : functions)
    f->semantic2(ctx);
  ctx->state = nullptr;
}

}}
