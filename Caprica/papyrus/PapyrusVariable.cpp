#include <papyrus/PapyrusVariable.h>

#include <papyrus/PapyrusObject.h>

namespace caprica { namespace papyrus {

void PapyrusVariable::buildPex(CapricaReportingContext& repCtx, pex::PexFile* file, pex::PexObject* obj) const {
  auto var = file->alloc->make<pex::PexVariable>();
  var->name = file->getString(name);
  var->typeName = type.buildPex(file);
  var->userFlags = userFlags.buildPex(file);
  var->defaultValue = defaultValue.buildPex(file);
  var->isConst = isConst();
  obj->variables.push_back(var);
}

void PapyrusVariable::semantic2(PapyrusResolutionContext* ctx) {
  if (ctx->object->isNative())
    ctx->reportingContext.error(location, "You cannot define variables in a Native script.");
  if (ctx->object->isConst() && !isConst())
    ctx->reportingContext.error(location, "You cannot define a non-const variable in a const script.");
  type = ctx->resolveType(type);
  if (type.type == PapyrusType::Kind::ResolvedObject && type.resolved.obj->isConst())
    ctx->reportingContext.error(location, "You cannot define a variable with the type of a Const script.");
  defaultValue = ctx->coerceDefaultValue(defaultValue, type);
}

}}
