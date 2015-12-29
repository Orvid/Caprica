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
  PapyrusType type{ };
  bool isAuto{ false };
  bool isReadOnly{ false };
  PapyrusUserFlags userFlags{ PapyrusUserFlags::None };
  PapyrusFunction* readFunction{ nullptr };
  PapyrusFunction* writeFunction{ nullptr };
  PapyrusValue defaultValue{ };

  parser::PapyrusFileLocation location{ };

  PapyrusProperty() = default;
  ~PapyrusProperty() {
    if (readFunction)
      delete readFunction;
    if (writeFunction)
      delete writeFunction;
  }

  void buildPex(pex::PexFile* file, pex::PexObject* obj) const {
    auto prop = new pex::PexProperty();
    prop->name = file->getString(name);
    prop->documentationString = file->getString(documentationComment);
    prop->typeName = type.buildPex(file);
    prop->userFlags = buildPexUserFlags(file, userFlags);
    prop->isAuto = isAuto;
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
      auto var = new pex::PexVariable();
      var->name = file->getString("::" + name + "_var");
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

    PapyrusIdentifier id;
    id.type = PapyrusIdentifierType::Property;
    id.name = name;
    id.prop = this;
    ctx->addIdentifier(id);
  }
};

}}
