#pragma once

#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/parser/PapyrusFileLocation.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusExpression abstract
{
  const parser::PapyrusFileLocation location;

  PapyrusExpression(const parser::PapyrusFileLocation& loc) : location(loc) { }
  virtual ~PapyrusExpression() = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const abstract;
  virtual void semantic(PapyrusResolutionContext* ctx) abstract;
  virtual PapyrusType resultType() const abstract;

  static PapyrusExpression* coerceExpression(PapyrusExpression* expr, const PapyrusType& target);

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
