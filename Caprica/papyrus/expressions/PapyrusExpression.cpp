#include <papyrus/expressions/PapyrusExpression.h>

#include <papyrus/expressions/PapyrusCastExpression.h>
#include <papyrus/expressions/PapyrusLiteralExpression.h>

namespace caprica { namespace papyrus { namespace expressions {

PapyrusExpression* PapyrusExpression::coerceExpression(PapyrusExpression* expr, PapyrusType target) {
  if (expr->resultType() != target) {
    // Do the cast at compile time for int->float conversion of literals
    // TODO: Only enable this with the optimization switch enabled, to be
    // consistent with the CK compiler.
    auto le = reinterpret_cast<PapyrusLiteralExpression*>(expr);
    if (expr->resultType() == PapyrusType::Int() && target == PapyrusType::Float() && le) {
      le->value.f = (float)le->value.i;
      le->value.type = PapyrusValueType::Float;
      return expr;
    }
    auto ce = new PapyrusCastExpression(expr->location);
    ce->innerExpression = expr;
    ce->targetType = target;
    return ce;
  }
  return expr;
}

}}}
