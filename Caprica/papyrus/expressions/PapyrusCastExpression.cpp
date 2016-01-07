#include <papyrus/expressions/PapyrusCastExpression.h>

namespace caprica { namespace papyrus { namespace expressions {

pex::PexValue PapyrusCastExpression::generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const {
  namespace op = caprica::pex::op;

  auto val = innerExpression->generateLoad(file, bldr);
  auto dest = bldr.allocTemp(file, targetType);
  bldr << location;
  bldr << op::cast{ dest, val };
  bldr.freeIfTemp(val);
  return dest;
}

void PapyrusCastExpression::semantic(PapyrusResolutionContext* ctx) {
  innerExpression->semantic(ctx);
  targetType = ctx->resolveType(targetType);
  
  if (innerExpression->resultType() == targetType)
    CapricaError::warning(location, "Unecessary cast from '%s' to '%s'.", innerExpression->resultType().prettyString().c_str(), targetType.prettyString().c_str());

  if (!PapyrusResolutionContext::canExplicitlyCast(innerExpression->resultType(), targetType)) {
    bool needsCast = true;
    if (!PapyrusResolutionContext::canImplicitlyCoerceExpression(innerExpression, targetType, needsCast))
      CapricaError::error(location, "Cannot convert from '%s' to '%s'!", innerExpression->resultType().prettyString().c_str(), targetType.prettyString().c_str());

    if (!needsCast)
      CapricaError::warning(location, "Unecessary cast from '%s'.", innerExpression->resultType().prettyString().c_str());
  }
}

PapyrusType PapyrusCastExpression::resultType() const {
  return targetType;
}

}}}
