#pragma once

#include <string>

#include <papyrus/PapyrusFunction.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/PapyrusValue.h>

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
  bool isConst{ false };
  bool isAutoReadOnly{ false };
  PapyrusUserFlags userFlags{ PapyrusUserFlags::None };
  PapyrusFunction* readFunction{ nullptr };
  PapyrusFunction* writeFunction{ nullptr };
  PapyrusValue defaultValue{ };

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
    prop->userFlags = userFlags;
    if (isConst) {
      auto func = new pex::PexFunction();
      func->returnTypeName = prop->typeName;
      func->documenationString = file->getString("");
      func->instructions.push_back(new pex::PexInstruction(pex::PexOpCode::Return, { defaultValue.buildPex(file) }));
      prop->readFunction = func;
    } else if (!readFunction && !writeFunction) {
      auto var = new pex::PexVariable();
      var->name = file->getString("::" + name + "_var");
      var->typeName = prop->typeName;
      var->userFlags = userFlags;
      var->defaultValue = defaultValue.buildPex(file);
      // TODO: Investigate how the official compiler distinguishes between a const
      // underlying var and a non-const. Some props are const, others are not.
      // In WorkshopParentScript, ::WorkshopCenterMarker_var is not marked const,
      // but ::WorkshopProduceFertilizer_var is, even though both are auto properties
      // and both initial values are none.
      var->isConst = true;
      prop->autoVar = var->name;
      prop->isAutoReadOnly = isAutoReadOnly;
      obj->variables.push_back(var);
    } else {
      if (readFunction)
        prop->readFunction = readFunction->buildPex(file, obj, prop);
      if (writeFunction)
        prop->writeFunction = writeFunction->buildPex(file, obj, prop);
    }
    obj->properties.push_back(prop);
  }
};

}}
