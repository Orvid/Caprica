#include <papyrus/expressions/PapyrusExpression.h>

#include <CapricaConfig.h>

#include <papyrus/expressions/PapyrusCastExpression.h>
#include <papyrus/expressions/PapyrusLiteralExpression.h>

namespace caprica { namespace papyrus { namespace expressions {

PapyrusExpression* PapyrusExpression::coerceExpression(PapyrusExpression* expr, const PapyrusType& target) {
  if (expr->resultType() != target) {
    // Do the cast at compile time for int->float conversion of literals
    if (CapricaConfig::enableOptimizations) {
      auto le = dynamic_cast<PapyrusLiteralExpression*>(expr);
      if (expr->resultType().type == PapyrusType::Kind::Int && target.type == PapyrusType::Kind::Float && le) {
        le->value.f = (float)le->value.i;
        le->value.type = PapyrusValueType::Float;
        return expr;
      }
    }
    auto ce = new PapyrusCastExpression(expr->location, target);
    ce->innerExpression = expr;
    return ce;
  }
  return expr;
}

}}}
