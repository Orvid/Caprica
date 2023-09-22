#pragma once

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/PapyrusType.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusCastExpression final : public PapyrusExpression {
  PapyrusExpression* innerExpression { nullptr };
  PapyrusType targetType;

  explicit PapyrusCastExpression(CapricaFileLocation loc, const PapyrusType& targ)
      : PapyrusExpression(loc), targetType(targ) { }
  explicit PapyrusCastExpression(CapricaFileLocation loc, PapyrusType&& targ)
      : PapyrusExpression(loc), targetType(std::move(targ)) { }
  PapyrusCastExpression(const PapyrusCastExpression&) = delete;
  virtual ~PapyrusCastExpression() override = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override;
  virtual void semantic(PapyrusResolutionContext* ctx) override;
  virtual PapyrusType resultType() const override;
  virtual PapyrusCastExpression* asCastExpression() override { return this; }
};

}}}
