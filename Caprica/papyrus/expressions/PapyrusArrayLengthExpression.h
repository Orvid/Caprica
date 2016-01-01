#pragma once

#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/parser/PapyrusFileLocation.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusArrayLengthExpression final : public PapyrusExpression
{
  PapyrusArrayLengthExpression(parser::PapyrusFileLocation loc) : PapyrusExpression(loc) { }
  virtual ~PapyrusArrayLengthExpression() override = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    throw std::runtime_error("This shouldn't be called!");
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    throw std::runtime_error("Illegal identifier: 'Length'!");
  }

  virtual PapyrusType resultType() const override {
    return PapyrusType::Int();
  }
};

}}}
