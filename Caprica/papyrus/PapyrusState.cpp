#include <papyrus/PapyrusState.h>

#include <papyrus/PapyrusObject.h>

namespace caprica { namespace papyrus {

static const PapyrusFunction* searchRootStateForFunction(const identifier_ref& name, const PapyrusObject* obj) {
  for (auto f : obj->getRootState()->functions) {
    if (idEq(f->name, name))
      return f;
  }

  if (auto parentObj = obj->tryGetParentClass())
    return searchRootStateForFunction(name, parentObj);
  return nullptr;
}

void PapyrusState::semantic(PapyrusResolutionContext* ctx) {
  ctx->state = this;
  for (auto f : functions)
    f->semantic(ctx);
  ctx->state = nullptr;
}

void PapyrusState::semantic2(PapyrusResolutionContext* ctx) {
  ctx->state = this;
  if (name != "") {
    if (ctx->object->isConst())
      ctx->reportingContext.error(location, "Named states aren't allowed on const scripts.");
    if (ctx->object->isNative())
      ctx->reportingContext.error(location, "Named states aren't allowed on Native scripts.");

    for (auto f : functions) {
      auto baseFunc = searchRootStateForFunction(f->name, ctx->object);
      if (!baseFunc)
        ctx->reportingContext.error(f->location, "Function '%s' cannot be defined in state '%s' without also being defined in the empty state!", f->name.to_string().c_str(), name.to_string().c_str());
      else if (!baseFunc->hasSameSignature(f))
        ctx->reportingContext.error(f->location, "The signature of the '%s' function (%s) in the '%s' state doesn't match the signature in the root state. The expected signature is '%s'.", f->name.to_string().c_str(), f->prettySignature().c_str(), name.to_string().c_str(), baseFunc->prettySignature().c_str());
    }
  }

  if (auto parentClass = ctx->object->tryGetParentClass()) {
    for (auto f : functions) {
      auto baseFunc = searchRootStateForFunction(f->name, parentClass);
      if (f->functionType == PapyrusFunctionType::Event && !baseFunc && !ctx->object->isNative())
        ctx->reportingContext.error(f->location, "Non-native scripts cannot define new events.");
      if (baseFunc && !baseFunc->hasSameSignature(f))
        ctx->reportingContext.error(f->location, "The signature of the '%s' function (%s) doesn't match the signature in the parent class '%s'. The expected signature is '%s'.", f->name.to_string().c_str(), f->prettySignature().c_str(), baseFunc->parentObject->name.to_string().c_str(), baseFunc->prettySignature().c_str());
    }
  }

  ctx->ensureNamesAreUnique(functions, "function");
  for (auto f : functions)
    f->semantic2(ctx);
  ctx->state = nullptr;
}

}}
