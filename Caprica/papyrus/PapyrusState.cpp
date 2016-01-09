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

  if (name != "") {
    for (auto f : functions) {
      if (!searchRootStateForFunction(f->name, ctx->object))
        CapricaError::error(f->location, "Function '%s' cannot be defined in state '%s' without also being defined in the empty state!", f->name.c_str(), name.c_str());
    }
  }
  ctx->state = nullptr;
}

}}
