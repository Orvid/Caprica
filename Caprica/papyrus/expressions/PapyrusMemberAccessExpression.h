#pragma once

#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusArrayLengthExpression.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/expressions/PapyrusIdentifierExpression.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusMemberAccessExpression final : public PapyrusExpression
{
  PapyrusExpression* baseExpression{ nullptr };
  PapyrusExpression* accessExpression{ nullptr };

  explicit PapyrusMemberAccessExpression(const CapricaFileLocation& loc) : PapyrusExpression(loc) { }
  virtual ~PapyrusMemberAccessExpression() override {
    if (baseExpression)
      delete baseExpression;
    if (accessExpression)
      delete accessExpression;
  }

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    namespace op = caprica::pex::op;
    pex::PexValue dest;
    if (auto id = accessExpression->as<PapyrusIdentifierExpression>()) {
      auto base = baseExpression->generateLoad(file, bldr);
      bldr << location;
      dest = id->identifier.generateLoad(file, bldr, pex::PexValue::Identifier::fromVar(base));
    } else if (auto al = accessExpression->as<PapyrusArrayLengthExpression>()) {
      auto base = baseExpression->generateLoad(file, bldr);
      bldr << location;
      dest = bldr.allocTemp(PapyrusType::Int(location));
      bldr << op::arraylength{ pex::PexValue::Identifier::fromVar(dest), pex::PexValue::Identifier::fromVar(base) };
    } else if (auto fc = accessExpression->as<PapyrusFunctionCallExpression>()) {
      dest = fc->generateLoad(file, bldr, baseExpression);
    } else {
      CapricaError::logicalFatal("Invalid access expression for PapyrusMemberAccessExpression!");
    }
    return dest;
  }

  void generateStore(pex::PexFile* file, pex::PexFunctionBuilder& bldr, pex::PexValue val) const {
    namespace op = caprica::pex::op;
    auto base = baseExpression->generateLoad(file, bldr);
    bldr << location;
    if (auto id = accessExpression->as<PapyrusIdentifierExpression>()) {
      id->identifier.generateStore(file, bldr, pex::PexValue::Identifier::fromVar(base), val);
    } else if (auto al = accessExpression->as<PapyrusArrayLengthExpression>()) {
      CapricaError::fatal(al->location, "You cannot assign to the .Length property of an array!");
    } else if (auto fc = accessExpression->as<PapyrusFunctionCallExpression>()) {
      CapricaError::fatal(fc->location, "You cannot assign to result of a function call!");
    } else {
      CapricaError::logicalFatal("Invalid access expression for PapyrusMemberAccessExpression!");
    }
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    if (auto fc = accessExpression->as<PapyrusFunctionCallExpression>()) {
      if (auto id = baseExpression->as<PapyrusIdentifierExpression>()) {
        id->identifier = ctx->tryResolveIdentifier(id->identifier);
        if (id->identifier.type == PapyrusIdentifierType::Unresolved) {
          auto tp = ctx->resolveType(PapyrusType::Unresolved(id->location, id->identifier.name));
          if (tp.type != PapyrusType::Kind::ResolvedObject)
            CapricaError::fatal(baseExpression->location, "Unresolved identifier '%s'!", id->identifier.name.c_str());
          fc->function = ctx->resolveFunctionIdentifier(tp, fc->function);
          fc->semantic(ctx);
          return;
        }
      }
      baseExpression->semantic(ctx);
      fc->function = ctx->resolveFunctionIdentifier(baseExpression->resultType(), fc->function);
      fc->semantic(ctx);
    } else {
      baseExpression->semantic(ctx);
      if (auto id = accessExpression->as<PapyrusIdentifierExpression>()) {
        id->identifier = ctx->resolveMemberIdentifier(baseExpression->resultType(), id->identifier);
        id->semantic(ctx);
      } else if (auto al = accessExpression->as<PapyrusArrayLengthExpression>()) {
        if (baseExpression->resultType().type != PapyrusType::Kind::Array)
          CapricaError::fatal(al->location, "Attempted to access the .Length property of a non-array value!");
      } else {
        CapricaError::logicalFatal("Invalid access expression for PapyrusMemberAccessExpression!");
      }
    }
  }

  virtual PapyrusType resultType() const override {
    return accessExpression->resultType();
  }
};

}}}
