#pragma once

#include <common/CapricaFileLocation.h>
#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusType.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusExpression abstract
{
  const CapricaFileLocation location;

  explicit PapyrusExpression(const CapricaFileLocation& loc) : location(loc) { }
  PapyrusExpression(const PapyrusExpression&) = delete;
  virtual ~PapyrusExpression() = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const abstract;
  virtual void semantic(PapyrusResolutionContext* ctx) abstract;
  virtual PapyrusType resultType() const abstract;

  template<typename T>
  T* as() {
    return dynamic_cast<T*>(this);
  }

  template<typename T>
  bool is() {
    return this->as<T>() != nullptr;
  }
};

}}}
