#pragma once

#include <string>

#include <common/CapricaFileLocation.h>
#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/PapyrusValue.h>

#include <pex/PexFile.h>
#include <pex/PexObject.h>
#include <pex/PexVariable.h>

namespace caprica { namespace papyrus {

struct PapyrusVariable final
{
  std::string name{ "" };
  PapyrusType type;
  PapyrusUserFlags userFlags{ };
  PapyrusValue defaultValue{ PapyrusValue::Default() };
  bool isConst{ false };

  CapricaFileLocation location;
  const PapyrusObject* parent{ nullptr };

  explicit PapyrusVariable(const CapricaFileLocation& loc, const PapyrusType& tp, const PapyrusObject* par) : location(loc), type(tp), parent(par) { }
  PapyrusVariable(const PapyrusVariable&) = delete;
  ~PapyrusVariable() = default;

  void buildPex(pex::PexFile* file, pex::PexObject* obj) const;
  void semantic(PapyrusResolutionContext* ctx);
};

}}
