#pragma once

#include <string>

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/parser/PapyrusFileLocation.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusIdentifierExpression final : public PapyrusExpression
{
  std::string identifier{ "" };

  PapyrusIdentifierExpression(parser::PapyrusFileLocation loc) : PapyrusExpression(loc) { }
  ~PapyrusIdentifierExpression() = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    bldr << location;
    return pex::PexValue::Identifier(file->getString(identifier));
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {

  }

  virtual PapyrusType resultType() const override {
    // TODO: Resolve in semantic and return here.
    // We don't currently have the ability to resolve this,
    // so return none and break stuff.
    return PapyrusType::None();
  }
};

}}}
