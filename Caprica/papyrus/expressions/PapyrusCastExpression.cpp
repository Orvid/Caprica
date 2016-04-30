#include <papyrus/expressions/PapyrusCastExpression.h>

namespace caprica { namespace papyrus { namespace expressions {

pex::PexValue PapyrusCastExpression::generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const {
  namespace op = caprica::pex::op;

  auto val = innerExpression->generateLoad(file, bldr);
  auto dest = bldr.allocTemp(targetType);
  bldr << location;
  bldr << op::cast{ dest, val };
  return dest;
}

void PapyrusCastExpression::semantic(PapyrusResolutionContext* ctx) {
  innerExpression->semantic(ctx);
  ctx->checkForPoison(innerExpression);
  targetType = ctx->resolveType(targetType);
  
  if (innerExpression->resultType() == targetType)
    ctx->reportingContext.warning_W4001_Unecessary_Cast(location, innerExpression->resultType().prettyString().c_str(), targetType.prettyString().c_str());

  if (!PapyrusResolutionContext::canExplicitlyCast(innerExpression->resultType(), targetType)) {
    if (!PapyrusResolutionContext::canImplicitlyCoerceExpression(innerExpression, targetType))
      ctx->reportingContext.error(location, "Cannot convert from '%s' to '%s'!", innerExpression->resultType().prettyString().c_str(), targetType.prettyString().c_str());
  }
}

PapyrusType PapyrusCastExpression::resultType() const {
  return targetType;
}

}}}
