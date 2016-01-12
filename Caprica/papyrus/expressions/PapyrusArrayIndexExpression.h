#pragma once

#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusArrayIndexExpression final : public PapyrusExpression
{
  PapyrusExpression* baseExpression{ nullptr };
  PapyrusExpression* indexExpression{ nullptr };

  PapyrusArrayIndexExpression(const CapricaFileLocation& loc) : PapyrusExpression(loc) { }
  virtual ~PapyrusArrayIndexExpression() override {
    if (baseExpression)
      delete baseExpression;
    if (indexExpression)
      delete indexExpression;
  }

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    auto base = baseExpression->generateLoad(file, bldr);
    auto idx = indexExpression->generateLoad(file, bldr);
    bldr << location;
    auto dest = bldr.allocTemp(this->resultType());
    bldr << op::arraygetelement{ dest, pex::PexValue::Identifier::fromVar(base), idx };
    return dest;
  }

  void generateStore(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue val) const {
    namespace op = caprica::pex::op;
    auto base = baseExpression->generateLoad(file, bldr);
    auto idx = indexExpression->generateLoad(file, bldr);
    bldr << location;
    bldr << op::arraysetelement{ pex::PexValue::Identifier::fromVar(base), idx, val };
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    baseExpression->semantic(ctx);
    if (baseExpression->resultType().type != PapyrusType::Kind::Array)
      CapricaError::error(baseExpression->location, "You can only index arrays! Got '%s'!", baseExpression->resultType().prettyString().c_str());
    indexExpression->semantic(ctx);
    indexExpression = PapyrusResolutionContext::coerceExpression(indexExpression, PapyrusType::Int(indexExpression->location));
  }

  virtual PapyrusType resultType() const override {
    auto res = baseExpression->resultType();
    if (res.type == PapyrusType::Kind::Array)
      return res.getElementType();
    return PapyrusType::None(location);
  }
};

}}}
