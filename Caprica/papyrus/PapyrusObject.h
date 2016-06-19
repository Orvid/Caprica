#pragma once

#include <string>
#include <vector>

#include <common/CapricaFileLocation.h>
#include <common/CaselessStringComparer.h>
#include <common/EngineLimits.h>

#include <papyrus/PapyrusCustomEvent.h>
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

enum class PapyrusResoultionState
{
  Unresolved,

  PreSemanticInProgress,
  PreSemanticCompleted,

  SemanticInProgress,
  SemanticCompleted,

  Semantic2InProgress,
  Semantic2Completed,
};

struct PapyrusObject final
{
  std::string name{ "" };
  std::string documentationString{ "" };
  PapyrusUserFlags userFlags{ };
  PapyrusType parentClass;
  PapyrusState* autoState{ nullptr };
  
  CapricaFileLocation location;

  std::vector<std::pair<CapricaFileLocation, std::string>> imports{ };
  std::vector<PapyrusStruct*> structs{ };
  std::vector<PapyrusVariable*> variables{ };
  std::vector<PapyrusPropertyGroup*> propertyGroups{ };
  std::vector<PapyrusState*> states{ };
  std::vector<PapyrusCustomEvent*> customEvents{ };

  bool isBetaOnly() const { return userFlags.isBetaOnly; }
  bool isConst() const { return userFlags.isConst; }
  bool isDebugOnly() const { return userFlags.isDebugOnly; }
  bool isNative() const { return userFlags.isNative; }

  explicit PapyrusObject(const CapricaFileLocation& loc, const PapyrusType& baseTp) : location(loc), parentClass(baseTp) {
    rootState = new PapyrusState(location);
    states.push_back(rootState);
  }
  PapyrusObject(const PapyrusObject&) = delete;
  ~PapyrusObject() {
    for (auto s : structs)
      delete s;
    for (auto v : variables)
      delete v;
    for (auto g : propertyGroups)
      delete g;
    for (auto s : states)
      delete s;
  }

  PapyrusPropertyGroup* getRootPropertyGroup();
  const PapyrusState* getRootState() const { return rootState; }
  PapyrusState* getRootState() { return rootState; }

  boost::string_ref loweredName() const {
    if (lowerName.size() == name.size())
      return lowerName;
    lowerName = name;
    identifierToLower(lowerName);
    return lowerName;
  }

  boost::string_ref getNamespaceName() const {
    auto pos = name.rfind(':');
    if (pos != std::string::npos)
      return boost::string_ref(name).substr(0, pos);
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

  PapyrusCompilationNode* compilationNode{ nullptr };
  PapyrusResoultionState resolutionState{ PapyrusResoultionState::Unresolved };
  PapyrusState* rootState{ nullptr };
  PapyrusPropertyGroup* rootPropertyGroup{ nullptr };
  mutable std::string lowerName{ };

  void checkForInheritedIdentifierConflicts(CapricaReportingContext& repCtx, caseless_unordered_identifier_ref_map<std::pair<bool, const char*>>& identMap, bool checkInheritedOnly) const;
};

}}
