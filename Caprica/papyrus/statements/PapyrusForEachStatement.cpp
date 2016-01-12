#include <papyrus/statements/PapyrusForEachStatement.h>

#include <papyrus/PapyrusFunction.h>

namespace caprica { namespace papyrus { namespace statements {

void PapyrusForEachStatement::buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const {
  namespace op = caprica::pex::op;
  pex::PexLabel* beforeCondition;
  bldr >> beforeCondition;
  pex::PexLabel* afterAll;
  bldr >> afterAll;
  bldr.pushBreakContinueScope(afterAll, beforeCondition);
  auto counter = bldr.allocLongLivedTemp(PapyrusType::Int(location));
  bldr << location;
  bldr << op::assign{ counter, pex::PexValue::Integer(0) };
  bldr << beforeCondition;
  auto baseVal = pex::PexValue::Identifier::fromVar(expressionToIterate->generateLoad(file, bldr));
  bldr << location;
  auto cTemp = bldr.allocTemp(PapyrusType::Int(location));
  if (expressionToIterate->resultType().type == PapyrusType::Kind::Array)
    bldr << op::arraylength{ cTemp, baseVal };
  else
    bldr << op::callmethod{ file->getString("GetCount"), baseVal, cTemp, { } };
  auto bTemp = bldr.allocTemp(PapyrusType::Bool(location));
  bldr << op::cmplt{ bTemp, counter, cTemp };
  bldr << op::jmpf{ bTemp, afterAll };

  auto loc = bldr.allocateLocal(declareStatement->name, declareStatement->type);
  auto bodyBaseVal = pex::PexValue::Identifier::fromVar(expressionToIterate->generateLoad(file, bldr));
  bldr << location;
  if (expressionToIterate->resultType().type == PapyrusType::Kind::Array)
    bldr << op::arraygetelement{ loc, bodyBaseVal, counter };
  else
    bldr << op::callmethod{ file->getString("GetAt"), bodyBaseVal, loc, { counter } };

  for (auto s : body)
    s->buildPex(file, bldr);

  bldr << location;
  bldr << op::iadd{ counter, counter, pex::PexValue::Integer(1) };
  bldr << op::jmp{ beforeCondition };
  bldr.freeLongLivedTemp(counter);
  bldr.popBreakContinueScope();
  bldr << afterAll;
}

void PapyrusForEachStatement::semantic(PapyrusResolutionContext* ctx) {
  expressionToIterate->semantic(ctx);
  auto elementType = [this, ctx](const PapyrusType& tp) -> PapyrusType {
    if (tp.type == PapyrusType::Kind::Array) {
      return tp.getElementType();
    } else if (tp.type == PapyrusType::Kind::ResolvedObject) {
      auto gCountId = ctx->tryResolveFunctionIdentifier(tp, PapyrusIdentifier::Unresolved(tp.location, "GetCount"));
      if (gCountId.type != PapyrusIdentifierType::Function) {
        CapricaError::error(tp.location, "Unable to iterate over a value of type '%s' as it does not implement GetCount!", tp.prettyString().c_str());
        return PapyrusType::None(tp.location);
      }
      if (gCountId.func->returnType.type != PapyrusType::Kind::Int) {
        CapricaError::error(tp.location, "Unable to iterate over a value of type '%s' because GetCount has a return type of '%s', expected 'Int'!", tp.prettyString().c_str(), gCountId.func->returnType.prettyString().c_str());
        return PapyrusType::None(tp.location);
      }
      this->getCountIdentifier = new PapyrusIdentifier(gCountId);

      auto gAtId = ctx->tryResolveFunctionIdentifier(tp, PapyrusIdentifier::Unresolved(tp.location, "GetAt"));
      if (gAtId.type != PapyrusIdentifierType::Function) {
        CapricaError::error(tp.location, "Unable to iterate over a value of type '%s' as it does not implement GetAt!", tp.prettyString().c_str());
        return PapyrusType::None(tp.location);
      }
      if (gAtId.func->parameters.size() != 1) {
        CapricaError::error(tp.location, "Unable to iterate over a value of type '%s' as its GetAt function does not accept exactly one parameter!", tp.prettyString().c_str());
        return PapyrusType::None(tp.location);
      }
      if (gAtId.func->parameters[0]->type != gCountId.func->returnType) {
        CapricaError::error(tp.location, "Unable to iterate over a value of type '%s' as its GetAt function has one parameter, but it is of type '%s', not 'Int'!", tp.prettyString().c_str(), gAtId.func->parameters[0]->type.prettyString().c_str());
        return PapyrusType::None(tp.location);
      }
      this->getAtIdentifier = new PapyrusIdentifier(gAtId);

      return gAtId.func->returnType;
    } else {
      CapricaError::error(tp.location, "Cannot iterate over a value of type '%s'!", tp.prettyString().c_str());
      return PapyrusType::None(tp.location);
    }
  }(expressionToIterate->resultType());

  ctx->pushBreakContinueScope();
  ctx->pushLocalVariableScope();
  if (declareStatement->isAuto) {
    declareStatement->type = elementType;
    declareStatement->isAuto = false;
  }
  declareStatement->semantic(ctx);
  // TODO: Perhaps allow declaring it as a parent object of the element type?
  if (declareStatement->type != elementType)
    CapricaError::error(declareStatement->type.location, "Cannot declare the loop variable as '%s', expected '%s'!", declareStatement->type.prettyString().c_str(), elementType.prettyString().c_str());
  for (auto s : body)
    s->semantic(ctx);
  ctx->popLocalVariableScope();
  ctx->popBreakContinueScope();
}

}}}
