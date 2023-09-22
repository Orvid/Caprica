#pragma once

#include <string>
#include <vector>

#include <common/CapricaFileLocation.h>
#include <common/CaselessStringComparer.h>
#include <common/EngineLimits.h>
#include <common/identifier_ref.h>
#include <common/IntrusiveLinkedList.h>

#include <papyrus/PapyrusCustomEvent.h>
#include <papyrus/PapyrusGuard.h>
#include <papyrus/PapyrusProperty.h>
#include <papyrus/PapyrusPropertyGroup.h>
#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusState.h>
#include <papyrus/PapyrusStruct.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/PapyrusVariable.h>

#include <pex/PexFile.h>
#include <pex/PexObject.h>

namespace caprica { namespace papyrus {

struct PapyrusCompilationNode;

enum class PapyrusResoultionState {
  Unresolved,

  PreSemanticInProgress,
  PreSemanticCompleted,

  SemanticInProgress,
  SemanticCompleted,

  Semantic2InProgress,
  Semantic2Completed,
};

struct PapyrusObject final {
  identifier_ref name { "" };
  identifier_ref documentationString { "" };
  PapyrusUserFlags userFlags {};
  PapyrusType parentClass;
  PapyrusState* autoState { nullptr };

  CapricaFileLocation location;

  std::vector<std::pair<CapricaFileLocation, identifier_ref>> imports {};
  IntrusiveLinkedList<PapyrusStruct> structs {};
  IntrusiveLinkedList<PapyrusVariable> variables {};
  IntrusiveLinkedList<PapyrusGuard> guards {};
  IntrusiveLinkedList<PapyrusPropertyGroup> propertyGroups {};
  IntrusiveLinkedList<PapyrusState> states {};
  IntrusiveLinkedList<PapyrusCustomEvent> customEvents {};

  bool isBetaOnly() const { return userFlags.isBetaOnly; }
  bool isConst() const { return userFlags.isConst; }
  bool isDebugOnly() const { return userFlags.isDebugOnly; }
  bool isNative() const { return userFlags.isNative; }

  explicit PapyrusObject(const CapricaFileLocation& loc, allocators::ChainedPool* alloc, const PapyrusType& baseTp)
      : location(loc), parentClass(baseTp) {
    rootState = alloc->make<PapyrusState>(location);
    states.push_back(rootState);
  }
  PapyrusObject(const PapyrusObject&) = delete;
  ~PapyrusObject() = default;

  PapyrusPropertyGroup* getRootPropertyGroup();
  const PapyrusState* getRootState() const { return rootState; }
  PapyrusState* getRootState() { return rootState; }

  void setName(identifier_ref nm) {
    name = nm;
    lowerName = name.to_string();
    identifierToLower(lowerName);
  }

  identifier_ref loweredName() const { return lowerName; }

  identifier_ref getNamespaceName() const {
    auto pos = name.rfind(':');
    if (pos != identifier_ref::npos)
      return identifier_ref(name).substr(0, pos);
    return "";
  }

  const PapyrusObject* tryGetParentClass() const;
  void buildPex(CapricaReportingContext& repCtx, pex::PexFile* file) const;
  void semantic(PapyrusResolutionContext* ctx);
  void semantic2(PapyrusResolutionContext* ctx);

  void preSemantic(PapyrusResolutionContext* ctx) {
    resolutionState = PapyrusResoultionState::PreSemanticInProgress;
    parentClass = ctx->resolveType(parentClass, true);
    resolutionState = PapyrusResoultionState::PreSemanticCompleted;
  }

  PapyrusObject* awaitSemantic() const;

private:
  friend PapyrusCompilationNode;
  friend IntrusiveLinkedList<PapyrusObject>;
  PapyrusObject* next { nullptr };

  PapyrusCompilationNode* compilationNode { nullptr };
  PapyrusResoultionState resolutionState { PapyrusResoultionState::Unresolved };
  PapyrusState* rootState { nullptr };
  PapyrusPropertyGroup* rootPropertyGroup { nullptr };
  std::string lowerName {};

  void
  checkForInheritedIdentifierConflicts(CapricaReportingContext& repCtx,
                                       caseless_unordered_identifier_ref_map<std::pair<bool, const char*>>& identMap,
                                       bool checkInheritedOnly) const;
};

}}
