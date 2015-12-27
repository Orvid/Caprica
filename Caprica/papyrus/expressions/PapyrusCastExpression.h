#pragma once

#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/parser/PapyrusFileLocation.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusCastExpression final : public PapyrusExpression
{
  PapyrusExpression* innerExpresion{ nullptr };
  PapyrusType targetType{ };

  PapyrusCastExpression(parser::PapyrusFileLocation loc) : PapyrusExpression(loc) { }
  ~PapyrusCastExpression() {
    if (innerExpresion)
      delete innerExpresion;
  }

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override;
  virtual void semantic(PapyrusResolutionContext* ctx) override;
  virtual PapyrusType resultType() const override;
};

}}}
