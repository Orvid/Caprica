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
      if (targetType.type == PapyrusType::Kind::ResolvedObject) {
        bldr.reportingContext.warning_W7005_Skyrim_Casting_None_Call_Result_To_Object(location,
                                                                          targetType.resolved.obj->name.to_string().c_str());
        val = bldr.getNoneLocal(location);
      } else if (targetType.type == PapyrusType::Kind::Bool) {
        bldr.reportingContext.warning_W7006_Skyrim_Casting_None_Call_Result_To_Bool(location);
        val = bldr.getNoneLocal(location);
      } else {
        bldr.reportingContext.fatal(location, "Cannot cast None method call result to '%s'!", targetType.prettyString().c_str());
      }
    }
  }

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

  if (!ctx->canExplicitlyCast(innerExpression->location, innerExpression->resultType(), targetType)) {
    if (!ctx->canImplicitlyCoerceExpression(innerExpression, targetType))
      ctx->reportingContext.error(location, "Cannot convert from '%s' to '%s'!", innerExpression->resultType().prettyString().c_str(), targetType.prettyString().c_str());
  }
}

PapyrusType PapyrusCastExpression::resultType() const {
  return targetType;
}

}}}
