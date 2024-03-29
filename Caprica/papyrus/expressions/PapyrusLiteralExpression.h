#pragma once

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/PapyrusValue.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusLiteralExpression final : public PapyrusExpression {
  PapyrusValue value;

  explicit PapyrusLiteralExpression(CapricaFileLocation loc, const PapyrusValue& val)
      : PapyrusExpression(loc), value(val) { }
  explicit PapyrusLiteralExpression(CapricaFileLocation loc, PapyrusValue&& val)
      : PapyrusExpression(loc), value(std::move(val)) { }
  PapyrusLiteralExpression(const PapyrusLiteralExpression&) = delete;
  virtual ~PapyrusLiteralExpression() override = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    bldr << location;
    return value.buildPex(file);
  }

  virtual void semantic(PapyrusResolutionContext*) override { }

  virtual PapyrusType resultType() const override { return value.getPapyrusType(); }

  virtual PapyrusLiteralExpression* asLiteralExpression() override { return this; }
};

}}}
