#pragma once

#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusArrayLengthExpression.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/expressions/PapyrusIdentifierExpression.h>
#include <papyrus/parser/PapyrusFileLocation.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusMemberAccessExpression final : public PapyrusExpression
{
  PapyrusExpression* baseExpression{ nullptr };
  PapyrusExpression* accessExpression{ nullptr };

  PapyrusMemberAccessExpression(parser::PapyrusFileLocation loc) : PapyrusExpression(loc) { }
  ~PapyrusMemberAccessExpression() {
    if (baseExpression)
      delete baseExpression;
    if (accessExpression)
      delete accessExpression;
  }

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    auto base = baseExpression->generateLoad(file, bldr);
    pex::PexValue dest;
    bldr << location;
    if (auto id = accessExpression->as<PapyrusIdentifierExpression>()) {
      dest = id->identifier.generateLoad(file, bldr, pex::PexValue::Identifier::fromVar(base));
    } else if (auto al = accessExpression->as<PapyrusArrayLengthExpression>()) {
      dest = bldr.allocTemp(file, PapyrusType::Int());
      bldr << op::arraylength{ pex::PexValue::Identifier::fromVar(dest), pex::PexValue::Identifier::fromVar(base) };
    } else {
      throw std::runtime_error("Invalid access expression for PapyrusMemberAccessExpression!");
    }
    bldr.freeIfTemp(base);
    return dest;
  }

  void generateStore(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue val) const {
    namespace op = caprica::pex::op;
    auto base = baseExpression->generateLoad(file, bldr);
    bldr << location;
    if (auto id = accessExpression->as<PapyrusIdentifierExpression>()) {
      id->identifier.generateStore(file, bldr, pex::PexValue::Identifier::fromVar(base), val);
    } else if (auto al = accessExpression->as<PapyrusArrayLengthExpression>()) {
      throw std::runtime_error("You cannot assign to the .Length property of an array!");
    } else {
      throw std::runtime_error("Invalid access expression for PapyrusMemberAccessExpression!");
    }
    bldr.freeIfTemp(base);
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    baseExpression->semantic(ctx);
    if (auto id = accessExpression->as<PapyrusIdentifierExpression>()) {
      id->identifier = ctx->resolveMemberIdentifier(baseExpression->resultType(), id->identifier);
      id->semantic(ctx);
    } else if (auto al = accessExpression->as<PapyrusArrayLengthExpression>()) {
      if (baseExpression->resultType().type != PapyrusType::Kind::Array)
        ctx->fatalError("Attempted to access the .Length property of a non-array value!");
    } else {
      throw std::runtime_error("Invalid access expression for PapyrusMemberAccessExpression!");
    }
  }

  virtual PapyrusType resultType() const override {
    return accessExpression->resultType();
  }
};

}}}
