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

  explicit PapyrusMemberAccessExpression(CapricaFileLocation loc) : PapyrusExpression(loc) { }
  PapyrusMemberAccessExpression(const PapyrusMemberAccessExpression&) = delete;
  virtual ~PapyrusMemberAccessExpression() override = default;

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
      CapricaReportingContext::logicalFatal("Invalid access expression for PapyrusMemberAccessExpression!");
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
      bldr.reportingContext.fatal(al->location, "You cannot assign to the .Length property of an array!");
    } else if (auto fc = accessExpression->as<PapyrusFunctionCallExpression>()) {
      bldr.reportingContext.fatal(fc->location, "You cannot assign to result of a function call!");
    } else {
      CapricaReportingContext::logicalFatal("Invalid access expression for PapyrusMemberAccessExpression!");
    }
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    // We don't explicitly use the access expression, so we don't
    // check it for poison.
    if (auto fc = accessExpression->as<PapyrusFunctionCallExpression>()) {
      if (auto id = baseExpression->as<PapyrusIdentifierExpression>()) {
        id->identifier = ctx->tryResolveIdentifier(id->identifier);
        if (id->identifier.type == PapyrusIdentifierType::Unresolved) {
          auto tp = ctx->resolveType(PapyrusType::Unresolved(id->location, id->identifier.name));
          if (tp.type != PapyrusType::Kind::ResolvedObject)
            ctx->reportingContext.fatal(baseExpression->location, "Unresolved identifier '%s'!", id->identifier.name.to_string().c_str());
          fc->function = ctx->resolveFunctionIdentifier(tp, fc->function, true);
          fc->semantic(ctx);
          return;
        }
      }
      baseExpression->semantic(ctx);
      ctx->checkForPoison(baseExpression);
      fc->function = ctx->resolveFunctionIdentifier(baseExpression->resultType(), fc->function);
      fc->semantic(ctx, baseExpression);
    } else {
      baseExpression->semantic(ctx);
      ctx->checkForPoison(baseExpression);
      if (auto id = accessExpression->as<PapyrusIdentifierExpression>()) {
        id->identifier = ctx->resolveMemberIdentifier(baseExpression->resultType(), id->identifier);
        id->semantic(ctx);
      } else if (auto al = accessExpression->as<PapyrusArrayLengthExpression>()) {
        if (baseExpression->resultType().type != PapyrusType::Kind::Array)
          ctx->reportingContext.fatal(al->location, "Attempted to access the .Length property of a non-array value!");
      } else {
        CapricaReportingContext::logicalFatal("Invalid access expression for PapyrusMemberAccessExpression!");
      }
    }
  }

  virtual PapyrusType resultType() const override {
    return accessExpression->resultType();
  }
};

}}}
