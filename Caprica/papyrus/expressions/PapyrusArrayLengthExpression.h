#pragma once

#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusArrayLengthExpression final : public PapyrusExpression
{
  explicit PapyrusArrayLengthExpression(const CapricaFileLocation& loc) : PapyrusExpression(loc) { }
  PapyrusArrayLengthExpression(const PapyrusArrayLengthExpression&) = delete;
  virtual ~PapyrusArrayLengthExpression() override = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    CapricaReportingContext::logicalFatal("This shouldn't be called!");
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    ctx->reportingContext.fatal(location, "Illegal identifier: 'Length'!");
  }

  virtual PapyrusType resultType() const override {
    return PapyrusType::Int(location);
  }

  virtual PapyrusArrayLengthExpression* asArrayLengthExpression() override {
    return this;
  }
};

}}}
