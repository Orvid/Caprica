#include <papyrus/PapyrusProperty.h>

#include <papyrus/PapyrusObject.h>

#include <pex/PexDebugFunctionInfo.h>
#include <pex/PexFunction.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexProperty.h>

namespace caprica { namespace papyrus {

void PapyrusProperty::buildPex(CapricaReportingContext& repCtx, pex::PexFile* file, pex::PexObject* obj) const {
  auto prop = new pex::PexProperty();
  prop->name = file->getString(name);
  prop->documentationString = file->getString(documentationComment);
  prop->typeName = type.buildPex(file);
  prop->userFlags = userFlags.buildPex(file, CapricaUserFlagsDefinition::ValidLocations::Property);
  if (isAuto() && isAutoReadOnly()) {
    prop->isReadable = true;
    auto func = new pex::PexFunction();
    func->returnTypeName = prop->typeName;
    func->documentationString = file->getString("");
    func->instructions.push_back(new pex::PexInstruction(pex::PexOpCode::Return, { defaultValue.buildPex(file) }));
    prop->readFunction = func;

    if (file->debugInfo) {
      auto fDebInfo = new pex::PexDebugFunctionInfo();
      fDebInfo->objectName = obj->name;
      fDebInfo->stateName = file->getString(""); // No state.
      fDebInfo->functionName = prop->name;
      fDebInfo->functionType = pex::PexDebugFunctionType::Getter;
      auto line = repCtx.getLocationLine(location);
      if (line > std::numeric_limits<uint16_t>::max())
        repCtx.fatal(location, "The file has too many lines for the debug info to be able to map correctly!");
      fDebInfo->instructionLineMap.push_back((uint16_t)line);
      file->debugInfo->functions.push_back(fDebInfo);
    }
  } else if (isAuto()) {
    prop->isAuto = true;
    prop->isReadable = true;
    prop->isWritable = true;
    auto var = new pex::PexVariable();
    var->name = file->getString(getAutoVarName());
    var->typeName = prop->typeName;
    var->userFlags = userFlags.buildPex(file, CapricaUserFlagsDefinition::ValidLocations::Variable);
    var->defaultValue = defaultValue.buildPex(file);
    var->isConst = isConst();
    prop->autoVar = var->name;
    obj->variables.push_back(var);
  } else {
    if (readFunction) {
      prop->isReadable = true;
      prop->readFunction = readFunction->buildPex(repCtx, file, obj, nullptr, prop->name);
    }
    if (writeFunction) {
      prop->isWritable = true;
      prop->writeFunction = writeFunction->buildPex(repCtx, file, obj, nullptr, prop->name);
    }
  }
  obj->properties.push_back(prop);
}

void PapyrusProperty::semantic(PapyrusResolutionContext* ctx) {
  type = ctx->resolveType(type);
  defaultValue = ctx->coerceDefaultValue(defaultValue, type);
  if (readFunction)
    readFunction->semantic(ctx);
  if (writeFunction)
    writeFunction->semantic(ctx);
}

void PapyrusProperty::semantic2(PapyrusResolutionContext* ctx) {
  if (ctx->object->isNative() && !isAutoReadOnly() && !readFunction && !writeFunction)
    ctx->reportingContext.error(location, "You cannot define auto properties on a Native script.");
  if (readFunction) {
    if (readFunction->isGlobal() || readFunction->isNative())
      ctx->reportingContext.error(readFunction->location, "A property function is not allowed to be global or native.");
    readFunction->semantic2(ctx);
  }
  if (writeFunction) {
    if (writeFunction->isGlobal() || writeFunction->isNative())
      ctx->reportingContext.error(writeFunction->location, "A property function is not allowed to be global or native.");
    writeFunction->semantic2(ctx);
  }
}

}}
