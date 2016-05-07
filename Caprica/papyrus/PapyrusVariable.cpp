#include <papyrus/PapyrusVariable.h>

#include <papyrus/PapyrusObject.h>

namespace caprica { namespace papyrus {

void PapyrusVariable::buildPex(CapricaReportingContext& repCtx, pex::PexFile* file, pex::PexObject* obj) const {
  auto var = new pex::PexVariable();
  var->name = file->getString(name);
  var->typeName = type.buildPex(file);
  var->userFlags = userFlags.buildPex(file);
  var->defaultValue = defaultValue.buildPex(file);
  var->isConst = isConst() || obj->isConst;
  obj->variables.push_back(var);
}

void PapyrusVariable::semantic(PapyrusResolutionContext* ctx) {
  if (ctx->object->isNative())
    ctx->reportingContext.error(location, "You cannot define variables in a Native script.");
  type = ctx->resolveType(type);
  defaultValue = ctx->coerceDefaultValue(defaultValue, type);
}

}}
