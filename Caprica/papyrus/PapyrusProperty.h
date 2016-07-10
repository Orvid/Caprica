#pragma once

#include <string>

#include <common/CapricaFileLocation.h>
#include <common/IntrusiveLinkedList.h>

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
  boost::string_ref name{ "" };
  boost::string_ref documentationComment{ "" };
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

  bool isFunctionBacked() const { return isAutoReadOnly() || readFunction || writeFunction; }

  explicit PapyrusProperty(CapricaFileLocation loc, PapyrusType&& tp, const PapyrusObject* par) : location(loc), type(std::move(tp)), parent(par) { }
  PapyrusProperty(const PapyrusProperty&) = delete;
  ~PapyrusProperty() = default;

  std::string getAutoVarName() const {
    return "::" + name.to_string() + "_var";
  }

  void buildPex(CapricaReportingContext& repCtx, pex::PexFile* file, pex::PexObject* obj) const;
  void semantic(PapyrusResolutionContext* ctx);
  void semantic2(PapyrusResolutionContext* ctx);

private:
  friend IntrusiveLinkedList<PapyrusProperty>;
  PapyrusProperty* next{ nullptr };
};

}}
