#include <papyrus/PapyrusState.h>

#include <papyrus/PapyrusObject.h>

namespace caprica { namespace papyrus {

static const PapyrusFunction* searchRootStateForFunction(const identifier_ref& name, const PapyrusObject* obj) {
  auto f = obj->getRootState()->functions.find(name);
  if (f != obj->getRootState()->functions.end())
    return f->second;

  if (auto parentObj = obj->tryGetParentClass())
    return searchRootStateForFunction(name, parentObj);
  return nullptr;
}

void PapyrusState::semantic(PapyrusResolutionContext* ctx) {
  ctx->state = this;
  for (auto f : functions)
    f.second->semantic(ctx);
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
      auto baseFunc = searchRootStateForFunction(f.second->name, ctx->object);
      if (!baseFunc)
        ctx->reportingContext.error(f.second->location, "Function '%s' cannot be defined in state '%s' without also being defined in the empty state!", f.second->name.to_string().c_str(), name.to_string().c_str());
      else if (!baseFunc->hasSameSignature(f.second))
        ctx->reportingContext.error(f.second->location, "The signature of the '%s' function (%s) in the '%s' state doesn't match the signature in the root state. The expected signature is '%s'.", f.second->name.to_string().c_str(), f.second->prettySignature().c_str(), name.to_string().c_str(), baseFunc->prettySignature().c_str());
    }
  }

  if (auto parentClass = ctx->object->tryGetParentClass()) {
    for (auto f : functions) {
      auto baseFunc = searchRootStateForFunction(f.second->name, parentClass);
      if (f.second->functionType == PapyrusFunctionType::Event && !baseFunc && !ctx->object->isNative()) {
        // TODO: Remove this skyrim hack
        if (conf::Papyrus::game == GameID::Skyrim){
          ctx->reportingContext.warning_W7000_Skyrim_Unknown_Event_On_Non_Native_Class(location, f.second->name.to_string().c_str(), ctx->object->name.to_string().c_str(), parentClass->name.to_string().c_str());
        } else {
          ctx->reportingContext.error(f.second->location, "Non-native scripts cannot define new events.");
        }
      }
      if (baseFunc && !baseFunc->hasSameSignature(f.second))
        ctx->reportingContext.error(f.second->location, "The signature of the '%s' function (%s) doesn't match the signature in the parent class '%s'. The expected signature is '%s'.", f.second->name.to_string().c_str(), f.second->prettySignature().c_str(), baseFunc->parentObject->name.to_string().c_str(), baseFunc->prettySignature().c_str());
    }
  }

  for (auto f : functions)
    f.second->semantic2(ctx);
  ctx->state = nullptr;
}

}}
