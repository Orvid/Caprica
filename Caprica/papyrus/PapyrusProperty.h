#pragma once

#include <limits>
#include <string>

#include <common/CapricaFileLocation.h>

#include <papyrus/PapyrusFunction.h>
#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/PapyrusValue.h>

#include <pex/PexFile.h>
#include <pex/PexObject.h>

namespace caprica { namespace papyrus {

struct PapyrusProperty final
{
  std::string name{ "" };
  std::string documentationComment{ "" };
  PapyrusType type;
  PapyrusUserFlags userFlags{ };
  PapyrusFunction* readFunction{ nullptr };
  PapyrusFunction* writeFunction{ nullptr };
  PapyrusValue defaultValue{ PapyrusValue::Default() };

  CapricaFileLocation location;
  const PapyrusObject* parent{ nullptr };

  bool isAuto() const { return userFlags.isAuto; }
  bool isAutoReadOnly() const { return userFlags.isAutoReadOnly; }
  bool isConst() const { return userFlags.isConst; }

  explicit PapyrusProperty(CapricaFileLocation loc, PapyrusType&& tp, const PapyrusObject* par) : location(loc), type(std::move(tp)), parent(par) { }
  PapyrusProperty(const PapyrusProperty&) = delete;
  ~PapyrusProperty() {
    if (readFunction)
      delete readFunction;
    if (writeFunction)
      delete writeFunction;
  }

  std::string getAutoVarName() const {
    return "::" + name + "_var";
  }

  void buildPex(CapricaReportingContext& repCtx, pex::PexFile* file, pex::PexObject* obj) const;
  void semantic(PapyrusResolutionContext* ctx);
  void semantic2(PapyrusResolutionContext* ctx);
};

}}
