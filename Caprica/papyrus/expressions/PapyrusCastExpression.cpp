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
  ctx->ensureCastable(innerExpression->resultType(), targetType);
}

PapyrusType PapyrusCastExpression::resultType() const {
  return targetType;
}

}}}
