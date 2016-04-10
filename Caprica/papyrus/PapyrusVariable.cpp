#include <papyrus/PapyrusVariable.h>

namespace caprica { namespace papyrus {

void PapyrusVariable::buildPex(pex::PexFile* file, pex::PexObject* obj) const {
  auto var = new pex::PexVariable();
  var->name = file->getString(name);
  var->typeName = type.buildPex(file);
  var->userFlags = userFlags.buildPex(file);
  var->defaultValue = defaultValue.buildPex(file);
  var->isConst = isConst() || obj->isConst;
  obj->variables.push_back(var);
}

void PapyrusVariable::semantic(PapyrusResolutionContext* ctx) {
  type = ctx->resolveType(type);
  defaultValue = PapyrusResolutionContext::coerceDefaultValue(defaultValue, type);
}

}}
