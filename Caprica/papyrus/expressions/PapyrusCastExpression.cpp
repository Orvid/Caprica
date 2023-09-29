#include <papyrus/expressions/PapyrusCastExpression.h>
#include <papyrus/expressions/PapyrusFunctionCallExpression.h>
#include <papyrus/PapyrusObject.h>
namespace caprica { namespace papyrus { namespace expressions {

pex::PexValue PapyrusCastExpression::generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const {
  namespace op = caprica::pex::op;

  auto val = innerExpression->generateLoad(file, bldr);
  auto dest = bldr.allocTemp(targetType);

  if (conf::Papyrus::game == GameID::Skyrim) {
    // the only invalid value that would be passed back to `val` here would come as a result
    // from a void function call returning ::NoneVar. You are apparently allowed to assign the results
    // of void function calls to objects and bools, so we need to check for that here.
    if (val.type == pex::PexValueType::Invalid) {
      if (!conf::Skyrim::skyrimAllowAssigningVoidMethodCallResult) {
        bldr.reportingContext.fatal(location,
                                    "Cannot cast None method call result to '{}'!",
                                    targetType);
      }
      switch (targetType.type) {
        case PapyrusType::Kind::ResolvedObject:
        case PapyrusType::Kind::ResolvedStruct:
        case PapyrusType::Kind::Bool:
        case PapyrusType::Kind::Var:
        case PapyrusType::Kind::String:
        case PapyrusType::Kind::Array:
        case PapyrusType::Kind::None:
          bldr.reportingContext.warning_W7005_Skyrim_Casting_None_Call_Result(location,
                                                                              targetType.prettyString());
          val = bldr.getNoneLocal(location);
          break;
        default:
          if (conf::Papyrus::allowImplicitNoneCastsToAnyType) {
            bldr.reportingContext.warning_W7005_Skyrim_Casting_None_Call_Result(location,
                                                                                targetType.prettyString());
            val = bldr.getNoneLocal(location);
            break;
          }
          bldr.reportingContext.fatal(location,
                                      "Cannot cast None method call result to '{}'!",
                                      targetType);
          break;
      }
    }
  }

  bldr << location;
  bldr << op::cast { dest, val };
  return dest;
}

void PapyrusCastExpression::semantic(PapyrusResolutionContext* ctx) {
  innerExpression->semantic(ctx);
  ctx->checkForPoison(innerExpression);
  targetType = ctx->resolveType(targetType);

  if (innerExpression->resultType() == targetType) {
    ctx->reportingContext.warning_W4001_Unecessary_Cast(location,
                                                        innerExpression->resultType().prettyString().c_str(),
                                                        targetType.prettyString().c_str());
  }

  if (!ctx->canExplicitlyCast(innerExpression->location, innerExpression->resultType(), targetType)) {
    if (!ctx->canImplicitlyCoerceExpression(innerExpression, targetType)) {
      ctx->reportingContext.error(location,
                                  "Cannot convert from '{}' to '{}'!",
                                  innerExpression->resultType(),
                                  targetType);
    }
  }
}

PapyrusType PapyrusCastExpression::resultType() const {
  return targetType;
}

}}}
