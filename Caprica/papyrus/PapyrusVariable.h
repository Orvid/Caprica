#pragma once

#include <common/CapricaFileLocation.h>
#include <common/CapricaReferenceState.h>
#include <common/identifier_ref.h>
#include <common/IntrusiveLinkedList.h>

#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/PapyrusValue.h>

#include <pex/PexFile.h>
#include <pex/PexObject.h>
#include <pex/PexVariable.h>

namespace caprica { namespace papyrus {

struct PapyrusVariable final {
  identifier_ref name { "" };
  PapyrusType type;
  PapyrusUserFlags userFlags {};
  PapyrusValue defaultValue { PapyrusValue::Default() };

  CapricaFileLocation location;
  const PapyrusObject* parent { nullptr };
  CapricaReferenceState referenceState {};

  bool isConst() const { return userFlags.isConst; }

  explicit PapyrusVariable(CapricaFileLocation loc, PapyrusType&& tp, const PapyrusObject* par)
      : location(loc), type(std::move(tp)), parent(par) { }
  PapyrusVariable(const PapyrusVariable&) = delete;
  ~PapyrusVariable() = default;

  void buildPex(CapricaReportingContext& repCtx, pex::PexFile* file, pex::PexObject* obj) const;
  void semantic2(PapyrusResolutionContext* ctx);

private:
  friend IntrusiveLinkedList<PapyrusVariable>;
  PapyrusVariable* next { nullptr };
};

}}
