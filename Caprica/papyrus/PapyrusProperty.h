#pragma once

#include <string>

#include <papyrus/PapyrusFunction.h>
#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/PapyrusValue.h>
#include <papyrus/parser/PapyrusFileLocation.h>

#include <pex/PexDebugFunctionInfo.h>
#include <pex/PexFile.h>
#include <pex/PexFunction.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexObject.h>
#include <pex/PexProperty.h>

namespace caprica { namespace papyrus {

struct PapyrusProperty final
{
  std::string name{ "" };
  std::string documentationComment{ "" };
  PapyrusType type;
  bool isAuto{ false };
  bool isReadOnly{ false };
  PapyrusUserFlags userFlags{ PapyrusUserFlags::None };
  PapyrusFunction* readFunction{ nullptr };
  PapyrusFunction* writeFunction{ nullptr };
  PapyrusValue defaultValue{ PapyrusValue::Default() };

  parser::PapyrusFileLocation location;

  PapyrusProperty(const parser::PapyrusFileLocation& loc, const PapyrusType& tp) : location(loc), type(tp) { }
  ~PapyrusProperty() {
    if (readFunction)
      delete readFunction;
    if (writeFunction)
      delete writeFunction;
  }

  std::string getAutoVarName() const {
    return "::" + name + "_var";
  }

  void buildPex(pex::PexFile* file, pex::PexObject* obj) const {
    auto prop = new pex::PexProperty();
    prop->name = file->getString(name);
    prop->documentationString = file->getString(documentationComment);
    prop->typeName = type.buildPex(file);
    prop->userFlags = buildPexUserFlags(file, userFlags);
    if (isAuto && isReadOnly) {
      prop->isReadable = true;
      auto func = new pex::PexFunction();
      func->returnTypeName = prop->typeName;
      func->documenationString = file->getString("");
      func->instructions.push_back(new pex::PexInstruction(pex::PexOpCode::Return, { defaultValue.buildPex(file) }));
      prop->readFunction = func;

      if (file->debugInfo) {
        auto fDebInfo = new pex::PexDebugFunctionInfo();
        fDebInfo->objectName = obj->name;
        fDebInfo->stateName = file->getString(""); // No state.
        fDebInfo->functionName = prop->name;
        fDebInfo->functionType = pex::PexDebugFunctionType::Getter;
        fDebInfo->instructionLineMap.push_back(location.buildPex());
        file->debugInfo->functions.push_back(fDebInfo);
      }
    } else if (isAuto) {
      prop->isAuto = true;
      prop->isReadable = true;
      prop->isWritable = true;
      auto var = new pex::PexVariable();
      var->name = file->getString(getAutoVarName());
      var->typeName = prop->typeName;
      var->userFlags = buildPexUserFlags(file, userFlags);
      var->defaultValue = defaultValue.buildPex(file);
      // TODO: Investigate how the official compiler distinguishes between a const
      // underlying var and a non-const. Some props are const, others are not.
      // In WorkshopParentScript, ::WorkshopCenterMarker_var is not marked const,
      // but ::WorkshopProduceFertilizer_var is, even though both are auto properties
      // and both initial values are none.
      var->isConst = true;
      prop->autoVar = var->name;
      obj->variables.push_back(var);
    } else {
      if (readFunction) {
        prop->isReadable = true;
        prop->readFunction = readFunction->buildPex(file, obj, nullptr, pex::PexDebugFunctionType::Getter, prop->name);
      }
      if (writeFunction) {
        prop->isWritable = true;
        prop->writeFunction = writeFunction->buildPex(file, obj, nullptr, pex::PexDebugFunctionType::Setter, prop->name);
      }
    }
    obj->properties.push_back(prop);
  }

  void semantic(PapyrusResolutionContext* ctx) {
    type = ctx->resolveType(type);
    ctx->prop = this;
    if (readFunction)
      readFunction->semantic(ctx);
    if (writeFunction)
      writeFunction->semantic(ctx);
    ctx->prop = nullptr;
    ctx->addIdentifier(PapyrusIdentifier::Property(location, this));
  }

  void semantic2(PapyrusResolutionContext* ctx) {
    ctx->prop = this;
    if (readFunction)
      readFunction->semantic2(ctx);
    if (writeFunction)
      writeFunction->semantic2(ctx);
    ctx->prop = nullptr;
  }
};

}}
