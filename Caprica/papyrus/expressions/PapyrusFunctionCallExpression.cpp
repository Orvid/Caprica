#include <papyrus/expressions/PapyrusFunctionCallExpression.h>

#include <cstring>

#include <papyrus/PapyrusFunction.h>
#include <papyrus/PapyrusObject.h>
#include <papyrus/expressions/PapyrusLiteralExpression.h>
#include <papyrus/expressions/PapyrusParentExpression.h>

namespace caprica { namespace papyrus { namespace expressions {

pex::PexValue PapyrusFunctionCallExpression::generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr, PapyrusExpression* base) const {
  if (!shouldEmit)
    return pex::PexValue::Invalid();

  namespace op = caprica::pex::op;
  if (function.type == PapyrusIdentifierType::BuiltinArrayFunction) {
    auto bVal = pex::PexValue::Identifier::fromVar(base->generateLoad(file, bldr));
    switch (function.arrayFuncKind) {
      case PapyrusBuiltinArrayFunctionKind::Find:
      {
        auto elem = arguments[0]->value->generateLoad(file, bldr);
        auto idx = arguments[1]->value->generateLoad(file, bldr);
        auto dest = bldr.allocTemp(resultType());
        bldr << location;
        bldr << op::arrayfindelement{ bVal, dest, elem, idx };
        return dest;
      }
      case PapyrusBuiltinArrayFunctionKind::FindStruct:
      {
        auto memberName = arguments[0]->value->as<PapyrusLiteralExpression>()->value.buildPex(file);
        auto elem = arguments[1]->value->generateLoad(file, bldr);
        auto idx = arguments[2]->value->generateLoad(file, bldr);
        auto dest = bldr.allocTemp(resultType());
        bldr << location;
        bldr << op::arrayfindstruct{ bVal, dest, memberName, elem, idx };
        return dest;
      }
      case PapyrusBuiltinArrayFunctionKind::RFind:
      {
        auto elem = arguments[0]->value->generateLoad(file, bldr);
        auto idx = arguments[1]->value->generateLoad(file, bldr);
        auto dest = bldr.allocTemp(resultType());
        bldr << location;
        bldr << op::arrayrfindelement{ bVal, dest, elem, idx };
        return dest;
      }
      case PapyrusBuiltinArrayFunctionKind::RFindStruct:
      {
        auto memberName = arguments[0]->value->as<PapyrusLiteralExpression>()->value.buildPex(file);
        auto elem = arguments[1]->value->generateLoad(file, bldr);
        auto idx = arguments[2]->value->generateLoad(file, bldr);
        auto dest = bldr.allocTemp(resultType());
        bldr << location;
        bldr << op::arrayrfindstruct{ bVal, dest, memberName, elem, idx };
        return dest;
      }
      case PapyrusBuiltinArrayFunctionKind::Add:
      {
        auto elem = arguments[0]->value->generateLoad(file, bldr);
        auto cnt = arguments[1]->value->generateLoad(file, bldr);
        bldr << location;
        bldr << op::arrayadd{ bVal, elem, cnt };
        return pex::PexValue::Invalid();
      }
      case PapyrusBuiltinArrayFunctionKind::Clear:
      {
          bldr << location;
          bldr << op::arrayclear{ bVal };
          return pex::PexValue::Invalid();
      }
      case PapyrusBuiltinArrayFunctionKind::Insert:
      {
        auto elem = arguments[0]->value->generateLoad(file, bldr);
        auto idx = arguments[1]->value->generateLoad(file, bldr);
        bldr << location;
        bldr << op::arrayinsert{ bVal, elem, idx };
        return pex::PexValue::Invalid();
      }
      case PapyrusBuiltinArrayFunctionKind::Remove:
      {
        auto idx = arguments[0]->value->generateLoad(file, bldr);
        auto cnt = arguments[1]->value->generateLoad(file, bldr);
        bldr << location;
        bldr << op::arrayremove{ bVal, idx, cnt };
        return pex::PexValue::Invalid();
      }
      case PapyrusBuiltinArrayFunctionKind::RemoveLast:
      {
        bldr << location;
        bldr << op::arrayremovelast{ bVal };
        return pex::PexValue::Invalid();
      }
      case PapyrusBuiltinArrayFunctionKind::Unknown:
        break;
    }
    CapricaReportingContext::logicalFatal("Unknown PapyrusBuiltinArrayFunctionKind!");
  } else {
    const auto getDest = [](pex::PexFunctionBuilder& bldr, const CapricaFileLocation& loc, const PapyrusType& tp) -> pex::PexValue::Identifier {
      if (tp.type == PapyrusType::Kind::None)
        return bldr.getNoneLocal(loc);
      else
        return bldr.allocTemp(tp);
    };
    auto dest = getDest(bldr, location, function.func->returnType);

    std::vector<pex::PexValue> args;
    for (auto param : arguments)
      args.emplace_back(param->value->generateLoad(file, bldr));
    bldr << location;
    if (function.func->isGlobal()) {
      bldr << op::callstatic{ file->getString(function.func->parentObject->loweredName()), file->getString(function.func->name), dest, std::move(args) };
    } else if (base && base->is<PapyrusParentExpression>()) {
      bldr << op::callparent{ file->getString(function.func->name), dest, std::move(args) };
    } else if (base) {
      auto bVal = base->generateLoad(file, bldr);
      bldr << op::callmethod{ file->getString(function.func->name), bVal, dest, std::move(args) };
    } else {
      bldr << op::callmethod{ file->getString(function.func->name), pex::PexValue::Identifier(file->getString("self")), dest, std::move(args) };
    }

    if (function.func->returnType.type == PapyrusType::Kind::None)
      return pex::PexValue::Invalid();
    return dest;
  }
}

void PapyrusFunctionCallExpression::semantic(PapyrusResolutionContext* ctx, PapyrusExpression* baseExpression) {
  if (baseExpression)
    ctx->checkForPoison(baseExpression);
  // TODO: Maybe pass baseExpression now that it's being passed to us?
  function = ctx->resolveFunctionIdentifier(PapyrusType::None(location), function);

  if (function.type == PapyrusIdentifierType::BuiltinArrayFunction) {
    for (size_t i = 0; i < arguments.size(); i++) {
      arguments[i]->value->semantic(ctx);
      ctx->checkForPoison(arguments[i]->value);
    }

    switch (function.arrayFuncKind) {
      case PapyrusBuiltinArrayFunctionKind::Find:
      {
        if (arguments.size() < 1 || arguments.size() > 2)
          ctx->reportingContext.fatal(location, "Expected either 1 or 2 parameters to 'Find'!");
        arguments[0]->value = ctx->coerceExpression(arguments[0]->value, *function.arrayFuncElementType);
        if (arguments.size() == 1) {
          auto p = ctx->allocator->make<Parameter>();
          p->value = ctx->allocator->make<PapyrusLiteralExpression>(location, PapyrusValue::Integer(location, 0));
          arguments.push_back(p);
        } else {
          if (arguments[1]->name != "" && !idEq(arguments[1]->name, "aiStartIndex"))
            ctx->reportingContext.error(arguments[1]->value->location, "Unknown argument '%s'! Was expecting 'aiStartIndex'!", arguments[1]->name.to_string().c_str());
          arguments[1]->value = ctx->coerceExpression(arguments[1]->value, PapyrusType::Int(arguments[1]->value->location));
        }
        return;
      }
      case PapyrusBuiltinArrayFunctionKind::FindStruct:
      {
        if (arguments.size() < 2 || arguments.size() > 3)
          ctx->reportingContext.fatal(location, "Expected either 2 or 3 parameters to 'FindStruct'!");
        if (arguments[0]->value->resultType().type != PapyrusType::Kind::String)
          ctx->reportingContext.fatal(location, "Expected the literal name of the struct member as a string to compare against!");
        
        auto memberName = arguments[0]->value->as<PapyrusLiteralExpression>()->value.s;
        PapyrusType elemType = PapyrusType::Default();
        for (auto m : function.arrayFuncElementType->resolvedStruct->members) {
          if (idEq(m->name, memberName)) {
            elemType = m->type;
            break;
          }
        }
        if (elemType.type == PapyrusType::Kind::None)
          ctx->reportingContext.fatal(arguments[0]->value->location, "Unknown member '%s' of struct '%s'!", memberName.to_string().c_str(), function.arrayFuncElementType->resolvedStruct->name.to_string().c_str());
        arguments[1]->value = ctx->coerceExpression(arguments[1]->value, elemType);

        if (arguments.size() == 2) {
          auto p = ctx->allocator->make<Parameter>();
          p->value = ctx->allocator->make<PapyrusLiteralExpression>(location, PapyrusValue::Integer(location, 0));
          arguments.push_back(p);
        } else {
          if (arguments[2]->name != "" && !idEq(arguments[2]->name, "aiStartIndex"))
            ctx->reportingContext.error(arguments[2]->value->location, "Unknown argument '%s'! Was expecting 'aiStartIndex'!", arguments[2]->name.to_string().c_str());
          arguments[2]->value = ctx->coerceExpression(arguments[2]->value, PapyrusType::Int(arguments[2]->value->location));
        }
        return;
      }
      case PapyrusBuiltinArrayFunctionKind::RFind:
      {
        if (arguments.size() < 1 || arguments.size() > 2)
          ctx->reportingContext.fatal(location, "Expected either 1 or 2 parameters to 'RFind'!");
        arguments[0]->value = ctx->coerceExpression(arguments[0]->value, *function.arrayFuncElementType);
        if (arguments.size() == 1) {
          auto p = ctx->allocator->make<Parameter>();
          p->value = ctx->allocator->make<PapyrusLiteralExpression>(location, PapyrusValue::Integer(location, -1));
          arguments.push_back(p);
        } else {
          if (arguments[1]->name != "" && !idEq(arguments[1]->name, "aiStartIndex"))
            ctx->reportingContext.error(arguments[1]->value->location, "Unknown argument '%s'! Was expecting 'aiStartIndex'!", arguments[1]->name.to_string().c_str());
          arguments[1]->value = ctx->coerceExpression(arguments[1]->value, PapyrusType::Int(arguments[1]->value->location));
        }
        return;
      }
      case PapyrusBuiltinArrayFunctionKind::RFindStruct:
      {
        if (arguments.size() < 2 || arguments.size() > 3)
          ctx->reportingContext.fatal(location, "Expected either 2 or 3 parameters to 'RFindStruct'!");
        if (arguments[0]->value->resultType().type != PapyrusType::Kind::String)
          ctx->reportingContext.fatal(location, "Expected the literal name of the struct member as a string to compare against!");

        auto memberName = arguments[0]->value->as<PapyrusLiteralExpression>()->value.s;
        PapyrusType elemType = PapyrusType::Default();
        for (auto m : function.arrayFuncElementType->resolvedStruct->members) {
          if (idEq(m->name, memberName)) {
            elemType = m->type;
            break;
          }
        }
        if (elemType.type == PapyrusType::Kind::None)
          ctx->reportingContext.fatal(arguments[0]->value->location, "Unknown member '%s' of struct '%s'!", memberName.to_string().c_str(), function.arrayFuncElementType->resolvedStruct->name.to_string().c_str());
        arguments[1]->value = ctx->coerceExpression(arguments[1]->value, elemType);

        if (arguments.size() == 2) {
          auto p = ctx->allocator->make<Parameter>();
          p->value = ctx->allocator->make<PapyrusLiteralExpression>(location, PapyrusValue::Integer(location, -1));
          arguments.push_back(p);
        } else {
          if (arguments[2]->name != "" && !idEq(arguments[2]->name, "aiStartIndex"))
            ctx->reportingContext.error(arguments[2]->value->location, "Unknown argument '%s'! Was expecting 'aiStartIndex'!", arguments[2]->name.to_string().c_str());
          arguments[2]->value = ctx->coerceExpression(arguments[2]->value, PapyrusType::Int(arguments[2]->value->location));
        }
        return;
      }
      case PapyrusBuiltinArrayFunctionKind::Add:
      {
        if (arguments.size() < 1 || arguments.size() > 2)
          ctx->reportingContext.fatal(location, "Expected either 1 or 2 parameters to 'Add'!");
        arguments[0]->value = ctx->coerceExpression(arguments[0]->value, *function.arrayFuncElementType);
        if (arguments.size() == 1) {
          auto p = ctx->allocator->make<Parameter>();
          p->value = ctx->allocator->make<PapyrusLiteralExpression>(location, PapyrusValue::Integer(location, 1));
          arguments.push_back(p);
        } else {
          if (arguments[1]->name != "" && !idEq(arguments[1]->name, "aiCount"))
            ctx->reportingContext.error(arguments[1]->value->location, "Unknown argument '%s'! Was expecting 'aiCount'!", arguments[1]->name.to_string().c_str());
          arguments[1]->value = ctx->coerceExpression(arguments[1]->value, PapyrusType::Int(arguments[1]->value->location));
        }
        return;
      }
      case PapyrusBuiltinArrayFunctionKind::Clear:
        if (arguments.size() != 0)
          ctx->reportingContext.fatal(location, "Expected 0 parameters to 'Clear'!");
        return;
      case PapyrusBuiltinArrayFunctionKind::Insert:
        if (arguments.size() != 2)
          ctx->reportingContext.fatal(location, "Expected 2 parameters to 'Insert'!");
        arguments[0]->value = ctx->coerceExpression(arguments[0]->value, *function.arrayFuncElementType);
        arguments[1]->value = ctx->coerceExpression(arguments[1]->value, PapyrusType::Int(arguments[1]->value->location));
        return;
      case PapyrusBuiltinArrayFunctionKind::Remove:
      {
        if (arguments.size() < 1 || arguments.size() > 2)
          ctx->reportingContext.fatal(location, "Expected either 1 or 2 parameters to 'Remove'!");
        arguments[0]->value = ctx->coerceExpression(arguments[0]->value, PapyrusType::Int(arguments[0]->value->location));
        if (arguments.size() == 1) {
          auto p = ctx->allocator->make<Parameter>();
          p->value = ctx->allocator->make<PapyrusLiteralExpression>(location, PapyrusValue::Integer(location, 1));
          arguments.push_back(p);
        } else {
          if (arguments[1]->name != "" && !idEq(arguments[1]->name, "aiCount"))
            ctx->reportingContext.error(arguments[1]->value->location, "Unknown argument '%s'! Was expecting 'aiCount'!", arguments[1]->name.to_string().c_str());
          arguments[1]->value = ctx->coerceExpression(arguments[1]->value, PapyrusType::Int(arguments[1]->value->location));
        }
        return;
      }
      case PapyrusBuiltinArrayFunctionKind::RemoveLast:
        if (arguments.size() != 0)
          ctx->reportingContext.fatal(location, "Expected 0 parameters to 'RemoveLast'!");
        return;
      case PapyrusBuiltinArrayFunctionKind::Unknown:
        break;
    }
    ctx->reportingContext.logicalFatal("Unknown PapyrusBuiltinArrayFunctionKind!");
  } else {
    assert(function.func != nullptr);

    if (function.func->returnType.isPoisoned(PapyrusType::PoisonKind::Beta)) {
      if (ctx->function == nullptr || !ctx->function->isBetaOnly()) {
        isPoisonedReturn = true;
        shouldEmit = !conf::CodeGeneration::disableBetaCode;
      }
    }
    if (function.func->returnType.isPoisoned(PapyrusType::PoisonKind::Debug)) {
      if (ctx->function == nullptr || !ctx->function->isDebugOnly()) {
        isPoisonedReturn = true;
        shouldEmit = !conf::CodeGeneration::disableDebugCode;
      }
    }

    bool hasNamedArgs = [&]() {
      for (auto a : arguments) {
        if (a->name != "")
          return true;
      }
      return false;
    }();
    if (hasNamedArgs || arguments.size() != function.func->parameters.size()) {
      // We may have default args to fill in.
      std::vector<Parameter*> newArgs;
      newArgs.resize(function.func->parameters.size());
      bool hadNamedArgs = false;
      for (size_t i = 0, baseI = 0; i < arguments.size(); i++, baseI++) {
        if (arguments[i]->name != "") {
          hadNamedArgs = true;
          for (size_t i2 = 0; i2 < function.func->parameters.size(); i2++) {
            if (idEq(function.func->parameters[i2]->name, arguments[i]->name)) {
              baseI = i2;
              goto ContinueOuterLoop;
            }
          }
          ctx->reportingContext.fatal(arguments[i]->value->location, "Unable to find a parameter named '%s'!", arguments[i]->name.to_string().c_str());
        }
        if (hadNamedArgs)
          ctx->reportingContext.fatal(arguments[i]->value->location, "No normal arguments are allowed after the first named argument!");
      ContinueOuterLoop:
        newArgs[baseI] = arguments[i];
      }

      for (size_t i = 0; i < newArgs.size(); i++) {
        if (newArgs[i] == nullptr) {
          if (function.func->parameters[i]->defaultValue.type == PapyrusValueType::Invalid)
            ctx->reportingContext.fatal(location, "Not enough arguments provided.");
          newArgs[i] = ctx->allocator->make<Parameter>();
          newArgs[i]->value = ctx->allocator->make<PapyrusLiteralExpression>(location, function.func->parameters[i]->defaultValue);
        }
      }
      arguments = std::move(newArgs);
    }

    for (size_t i = 0; i < arguments.size(); i++) {
      arguments[i]->value->semantic(ctx);
      ctx->checkForPoison(arguments[i]->value);
      switch (function.func->parameters[i]->type.type) {
        case PapyrusType::Kind::CustomEventName:
        case PapyrusType::Kind::ScriptEventName:
        {
          bool isCustomEvent = function.func->parameters[i]->type.type == PapyrusType::Kind::CustomEventName;

          auto le = arguments[i]->value->as<expressions::PapyrusLiteralExpression>();
          if (!le || le->value.type != PapyrusValueType::String) {
            ctx->reportingContext.error(arguments[i]->value->location, "Argument %zu must be string literal.", i);
            continue;
          }

          auto baseType = [&]() -> PapyrusType {
            if (i != 0)
              return arguments[i - 1]->value->resultType();
            if (baseExpression == nullptr)
              return PapyrusType::ResolvedObject(ctx->object->location, ctx->object);
            return baseExpression->resultType();
          }();
          if (baseType.type != PapyrusType::Kind::ResolvedObject)
            goto EventResolutionError;

          if (!isCustomEvent && ctx->tryResolveEvent(baseType.resolvedObject, le->value.s)) {
            continue;
          } else if (auto ev = ctx->tryResolveCustomEvent(baseType.resolvedObject, le->value.s)) {
            le->value.s = ctx->allocator->allocateString(ev->parentObject->name.to_string() + "_" + le->value.s.to_string());
            continue;
          }

        EventResolutionError:
          ctx->reportingContext.error(arguments[i]->value->location, "Unable to resolve %s event named '%s' in '%s' or one of its parents.", isCustomEvent ? "a custom" : "an", le->value.s.to_string().c_str(), baseType.prettyString().c_str());
          break;
        }
        default:
          break;
      }
    }

    // We need the semantic pass to have run for the args, but we can't have them coerced until after
    // we've transformed CustomEventName and ScriptEventName parameters.
    for (size_t i = 0; i < arguments.size(); i++) {
      if (function.func->parameters[i]->type.type == PapyrusType::Kind::CustomEventName) {
        arguments[i]->value = ctx->coerceExpression(arguments[i]->value, PapyrusType::String(function.func->parameters[i]->type.location));
      } else if (function.func->parameters[i]->type.type == PapyrusType::Kind::ScriptEventName) {
        arguments[i]->value = ctx->coerceExpression(arguments[i]->value, PapyrusType::String(function.func->parameters[i]->type.location));
      } else {
        arguments[i]->value = ctx->coerceExpression(arguments[i]->value, function.func->parameters[i]->type);
      }
    }

    if (function.func->name == "GotoState" && arguments.size() == 1) {
      auto le = arguments[0]->value->as<PapyrusLiteralExpression>();
      if (le && le->value.type == PapyrusValueType::String) {
        auto targetStateName = le->value.s;
        if (!ctx->tryResolveState(targetStateName))
          ctx->reportingContext.warning_W4003_State_Doesnt_Exist(le->location, targetStateName.to_string().c_str());
      }
    }
  }
}

PapyrusType PapyrusFunctionCallExpression::resultType() const {
  if (function.type == PapyrusIdentifierType::BuiltinArrayFunction) {
    switch (function.arrayFuncKind) {
      case PapyrusBuiltinArrayFunctionKind::Find:
      case PapyrusBuiltinArrayFunctionKind::FindStruct:
      case PapyrusBuiltinArrayFunctionKind::RFind:
      case PapyrusBuiltinArrayFunctionKind::RFindStruct:
        return PapyrusType::Int(location);
      default:
        return PapyrusType::None(location);
    }
  } else {
    if (isPoisonedReturn)
      return PapyrusType::PoisonedNone(location, function.func->returnType);
    return function.func->returnType;
  }
}

}}}
