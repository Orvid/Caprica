#pragma once

#include <common/CapricaFileLocation.h>
#include <common/identifier_ref.h>
#include <common/IntrusiveLinkedList.h>

#include <papyrus/PapyrusIdentifier.h>
#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusValue.h>

#include <pex/PexFile.h>
#include <pex/PexFunction.h>
#include <pex/PexFunctionParameter.h>
#include <pex/PexObject.h>
#include <pex/PexState.h>

namespace caprica { namespace papyrus {

struct PapyrusLockParameter final {
  identifier_ref name { "" };
  const size_t index;
  const PapyrusGuard* guard { nullptr };

  const CapricaFileLocation location;

  explicit PapyrusLockParameter(CapricaFileLocation loc, size_t idx) : location(loc), index(idx) { }
  PapyrusLockParameter(const PapyrusLockParameter&) = delete;
  ~PapyrusLockParameter() = default;

  void semantic(PapyrusResolutionContext* ctx) {
    auto id = ctx->resolveIdentifier(PapyrusIdentifier::Unresolved(location, name));
    assert(id.type == PapyrusIdentifierType::Guard);
    guard = id.res.guard;
    // guard = const_cast<PapyrusGuard*>(id.res.guard);
  }

private:
  friend IntrusiveLinkedList<PapyrusLockParameter>;
  PapyrusLockParameter* next { nullptr };
};

}}
