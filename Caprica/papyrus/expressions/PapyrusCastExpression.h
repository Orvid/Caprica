#pragma once

#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusCastExpression final : public PapyrusExpression
{
  PapyrusExpression* innerExpression{ nullptr };
  PapyrusType targetType;

  explicit PapyrusCastExpression(const CapricaFileLocation& loc, const PapyrusType& targ) : PapyrusExpression(loc), targetType(targ) { }
  PapyrusCastExpression(const PapyrusCastExpression&) = delete;
  virtual ~PapyrusCastExpression() override {
    if (innerExpression)
      delete innerExpression;
  }

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override;
  virtual void semantic(PapyrusResolutionContext* ctx) override;
  virtual PapyrusType resultType() const override;
};

}}}
