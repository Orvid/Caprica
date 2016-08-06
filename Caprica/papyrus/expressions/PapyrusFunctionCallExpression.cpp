#include <papyrus/expressions/PapyrusFunctionCallExpression.h>

#include <cstring>

#include <papyrus/PapyrusFunction.h>
#include <papyrus/PapyrusObject.h>
#include <papyrus/expressions/PapyrusLiteralExpression.h>
#include <papyrus/expressions/PapyrusParentExpression.h>

namespace caprica { namespace papyrus { namespace expressions {

static constexpr size_t MaxBuiltinArrayFunctionArgumentCount = 3;

pex::PexValue PapyrusFunctionCallExpression::generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr, PapyrusExpression* base) const {
  if (!shouldEmit)
    return pex::PexValue::Invalid();

  namespace op = caprica::pex::op;
  if (function.type == PapyrusIdentifierType::BuiltinArrayFunction) {
    assert(arguments.size() <= MaxBuiltinArrayFunctionArgumentCount);
    const Parameter* args[MaxBuiltinArrayFunctionArgumentCount] = { nullptr, nullptr, nullptr };
    auto cur = arguments.begin();
    for (size_t i = 0; i < arguments.size(); i++) {
      args[i] = *cur;
      ++cur;
    }

    auto bVal = pex::PexValue::Identifier::fromVar(base->generateLoad(file, bldr));
    switch (function.arrayFuncKind) {
      case PapyrusBuiltinArrayFunctionKind::Find:
      {
        auto elem = args[0]->value->generateLoad(file, bldr);
        auto idx = args[1]->value->generateLoad(file, bldr);
        auto dest = bldr.allocTemp(resultType());
        bldr << location;
        bldr << op::arrayfindelement{ bVal, dest, elem, idx };
        return dest;
      }
      case PapyrusBuiltinArrayFunctionKind::FindStruct:
      {
        auto memberName = args[0]->value->as<PapyrusLiteralExpression>()->value.buildPex(file);
        auto elem = args[1]->value->generateLoad(file, bldr);
        auto idx = args[2]->value->generateLoad(file, bldr);
        auto dest = bldr.allocTemp(resultType());
        bldr << location;
        bldr << op::arrayfindstruct{ bVal, dest, memberName, elem, idx };
        return dest;
      }
      case PapyrusBuiltinArrayFunctionKind::RFind:
      {
        auto elem = args[0]->value->generateLoad(file, bldr);
        auto idx = args[1]->value->generateLoad(file, bldr);
        auto dest = bldr.allocTemp(resultType());
        bldr << location;
        bldr << op::arrayrfindelement{ bVal, dest, elem, idx };
        return dest;
      }
      case PapyrusBuiltinArrayFunctionKind::RFindStruct:
      {
        auto memberName = args[0]->value->as<PapyrusLiteralExpression>()->value.buildPex(file);
        auto elem = args[1]->value->generateLoad(file, bldr);
        auto idx = args[2]->value->generateLoad(file, bldr);
        auto dest = bldr.allocTemp(resultType());
        bldr << location;
        bldr << op::arrayrfindstruct{ bVal, dest, memberName, elem, idx };
        return dest;
      }
      case PapyrusBuiltinArrayFunctionKind::Add:
      {
        auto elem = args[0]->value->generateLoad(file, bldr);
        auto cnt = args[1]->value->generateLoad(file, bldr);
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
        auto elem = args[0]->value->generateLoad(file, bldr);
        auto idx = args[1]->value->generateLoad(file, bldr);
        bldr << location;
        bldr << op::arrayinsert{ bVal, elem, idx };
        return pex::PexValue::Invalid();
      }
      case PapyrusBuiltinArrayFunctionKind::Remove:
      {
        auto idx = args[0]->value->generateLoad(file, bldr);
        auto cnt = args[1]->value->generateLoad(file, bldr);
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

    IntrusiveLinkedList<pex::PexValue> args;
    for (auto param : arguments)
      args.push_back(file->alloc->make<pex::PexValue>(param->value->generateLoad(file, bldr)));
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
    for (auto a : arguments) {
      a->value->semantic(ctx);
      ctx->checkForPoison(a->value);
    }

    Parameter* args[MaxBuiltinArrayFunctionArgumentCount] = { nullptr, nullptr, nullptr };
    const auto getArgs = [&](const char* funcName, size_t argCountMin, size_t argCountMax = 0) {
      assert(arguments.size() <= MaxBuiltinArrayFunctionArgumentCount);
      if (argCountMax != 0) {
        if (arguments.size() < argCountMin || arguments.size() > argCountMax)
          ctx->reportingContext.fatal(location, "Expected either %ull or %ull parameters to '%s'!", argCountMin, argCountMax, funcName);
      } else {
        if (arguments.size() != argCountMin)
          ctx->reportingContext.fatal(location, "Expected %ull parameters to '%s'!", argCountMin, funcName);
      }
      auto cur = arguments.begin();
      for (size_t i = 0; i < arguments.size(); i++) {
        args[i] = *cur;
        ++cur;
      }
    };

    switch (function.arrayFuncKind) {
      case PapyrusBuiltinArrayFunctionKind::Find:
      {
        getArgs("Find", 1, 2);
        args[0]->value = ctx->coerceExpression(args[0]->value, *function.arrayFuncElementType);
        if (arguments.size() == 1) {
          auto p = ctx->allocator->make<Parameter>();
          p->value = ctx->allocator->make<PapyrusLiteralExpression>(location, PapyrusValue::Integer(location, 0));
          arguments.push_back(p);
        } else {
          if (args[1]->name != "" && !idEq(args[1]->name, "aiStartIndex"))
            ctx->reportingContext.error(args[1]->value->location, "Unknown argument '%s'! Was expecting 'aiStartIndex'!", args[1]->name.to_string().c_str());
          args[1]->value = ctx->coerceExpression(args[1]->value, PapyrusType::Int(args[1]->value->location));
        }
        return;
      }
      case PapyrusBuiltinArrayFunctionKind::FindStruct:
      {
        getArgs("FindStruct", 2, 3);
        if (args[0]->value->resultType().type != PapyrusType::Kind::String)
          ctx->reportingContext.fatal(location, "Expected the literal name of the struct member as a string to compare against!");
        
        auto memberName = args[0]->value->as<PapyrusLiteralExpression>()->value.s;
        PapyrusType elemType = PapyrusType::Default();
        for (auto m : function.arrayFuncElementType->resolvedStruct->members) {
          if (idEq(m->name, memberName)) {
            elemType = m->type;
            break;
          }
        }
        if (elemType.type == PapyrusType::Kind::None)
          ctx->reportingContext.fatal(args[0]->value->location, "Unknown member '%s' of struct '%s'!", memberName.to_string().c_str(), function.arrayFuncElementType->resolvedStruct->name.to_string().c_str());
        args[1]->value = ctx->coerceExpression(args[1]->value, elemType);

        if (arguments.size() == 2) {
          auto p = ctx->allocator->make<Parameter>();
          p->value = ctx->allocator->make<PapyrusLiteralExpression>(location, PapyrusValue::Integer(location, 0));
          arguments.push_back(p);
        } else {
          if (args[2]->name != "" && !idEq(args[2]->name, "aiStartIndex"))
            ctx->reportingContext.error(args[2]->value->location, "Unknown argument '%s'! Was expecting 'aiStartIndex'!", args[2]->name.to_string().c_str());
          args[2]->value = ctx->coerceExpression(args[2]->value, PapyrusType::Int(args[2]->value->location));
        }
        return;
      }
      case PapyrusBuiltinArrayFunctionKind::RFind:
      {
        getArgs("RFind", 1, 2);
        args[0]->value = ctx->coerceExpression(args[0]->value, *function.arrayFuncElementType);
        if (arguments.size() == 1) {
          auto p = ctx->allocator->make<Parameter>();
          p->value = ctx->allocator->make<PapyrusLiteralExpression>(location, PapyrusValue::Integer(location, -1));
          arguments.push_back(p);
        } else {
          if (args[1]->name != "" && !idEq(args[1]->name, "aiStartIndex"))
            ctx->reportingContext.error(args[1]->value->location, "Unknown argument '%s'! Was expecting 'aiStartIndex'!", args[1]->name.to_string().c_str());
          args[1]->value = ctx->coerceExpression(args[1]->value, PapyrusType::Int(args[1]->value->location));
        }
        return;
      }
      case PapyrusBuiltinArrayFunctionKind::RFindStruct:
      {
        getArgs("RFindStruct", 2, 3);
        if (args[0]->value->resultType().type != PapyrusType::Kind::String)
          ctx->reportingContext.fatal(location, "Expected the literal name of the struct member as a string to compare against!");

        auto memberName = args[0]->value->as<PapyrusLiteralExpression>()->value.s;
        PapyrusType elemType = PapyrusType::Default();
        for (auto m : function.arrayFuncElementType->resolvedStruct->members) {
          if (idEq(m->name, memberName)) {
            elemType = m->type;
            break;
          }
        }
        if (elemType.type == PapyrusType::Kind::None)
          ctx->reportingContext.fatal(args[0]->value->location, "Unknown member '%s' of struct '%s'!", memberName.to_string().c_str(), function.arrayFuncElementType->resolvedStruct->name.to_string().c_str());
        args[1]->value = ctx->coerceExpression(args[1]->value, elemType);

        if (arguments.size() == 2) {
          auto p = ctx->allocator->make<Parameter>();
          p->value = ctx->allocator->make<PapyrusLiteralExpression>(location, PapyrusValue::Integer(location, -1));
          arguments.push_back(p);
        } else {
          if (args[2]->name != "" && !idEq(args[2]->name, "aiStartIndex"))
            ctx->reportingContext.error(args[2]->value->location, "Unknown argument '%s'! Was expecting 'aiStartIndex'!", args[2]->name.to_string().c_str());
          args[2]->value = ctx->coerceExpression(args[2]->value, PapyrusType::Int(args[2]->value->location));
        }
        return;
      }
      case PapyrusBuiltinArrayFunctionKind::Add:
      {
        getArgs("Add", 1, 2);
        args[0]->value = ctx->coerceExpression(args[0]->value, *function.arrayFuncElementType);
        if (arguments.size() == 1) {
          auto p = ctx->allocator->make<Parameter>();
          p->value = ctx->allocator->make<PapyrusLiteralExpression>(location, PapyrusValue::Integer(location, 1));
          arguments.push_back(p);
        } else {
          if (args[1]->name != "" && !idEq(args[1]->name, "aiCount"))
            ctx->reportingContext.error(args[1]->value->location, "Unknown argument '%s'! Was expecting 'aiCount'!", args[1]->name.to_string().c_str());
          args[1]->value = ctx->coerceExpression(args[1]->value, PapyrusType::Int(args[1]->value->location));
        }
        return;
      }
      case PapyrusBuiltinArrayFunctionKind::Clear:
        getArgs("Clear", 0);
        return;
      case PapyrusBuiltinArrayFunctionKind::Insert:
        getArgs("Insert", 2);
        args[0]->value = ctx->coerceExpression(args[0]->value, *function.arrayFuncElementType);
        args[1]->value = ctx->coerceExpression(args[1]->value, PapyrusType::Int(args[1]->value->location));
        return;
      case PapyrusBuiltinArrayFunctionKind::Remove:
      {
        getArgs("Remove", 1, 2);
        args[0]->value = ctx->coerceExpression(args[0]->value, PapyrusType::Int(args[0]->value->location));
        if (arguments.size() == 1) {
          auto p = ctx->allocator->make<Parameter>();
          p->value = ctx->allocator->make<PapyrusLiteralExpression>(location, PapyrusValue::Integer(location, 1));
          arguments.push_back(p);
        } else {
          if (args[1]->name != "" && !idEq(args[1]->name, "aiCount"))
            ctx->reportingContext.error(args[1]->value->location, "Unknown argument '%s'! Was expecting 'aiCount'!", args[1]->name.to_string().c_str());
          args[1]->value = ctx->coerceExpression(args[1]->value, PapyrusType::Int(args[1]->value->location));
        }
        return;
      }
      case PapyrusBuiltinArrayFunctionKind::RemoveLast:
        getArgs("RemoveLast", 0);
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

    const auto hasNamedArgs = [&]() {
      for (auto a : arguments) {
        if (a->name != "")
          return true;
      }
      return false;
    };
    if (arguments.size() != function.func->parameters.size() || hasNamedArgs()) {
      // We may have default args to fill in.
      IntrusiveLinkedList<Parameter> newArgs;
      bool hadNamedArgs = false;
      auto iter = arguments.begin();
      for (size_t baseI = 0; iter != arguments.end(); baseI++) {
        if (iter->name != "") {
          hadNamedArgs = true;
          auto newArgsIter = newArgs.beginInsertable();
          size_t newArgIndex = 0;
          for (auto p : function.func->parameters) {
            if (idEq(p->name, iter->name)) {
              auto a = *iter;
              ++iter;
              a->argIndex = p->index;
              newArgs.insertBefore(newArgsIter, a);
              goto ContinueOuterLoop;
            }
            if (newArgsIter != newArgs.endInsertable() && newArgsIter->argIndex <= p->index)
              ++newArgsIter;
            newArgIndex++;
          }
          ctx->reportingContext.fatal(iter->value->location, "Unable to find a parameter named '%s'!", iter->name.to_string().c_str());
        }
        if (hadNamedArgs)
          ctx->reportingContext.fatal(iter->value->location, "No normal arguments are allowed after the first named argument!");
        auto a = *iter;
        a->argIndex = baseI;
        ++iter;
        newArgs.push_back(a);
      ContinueOuterLoop:
        continue;
      }

      auto newArgsIter = newArgs.beginInsertable();
      for (auto p : function.func->parameters) {
        if (newArgsIter == newArgs.endInsertable() || newArgsIter->argIndex != p->index) {
          if (p->defaultValue.type == PapyrusValueType::Invalid)
            ctx->reportingContext.fatal(location, "Not enough arguments provided.");
          auto newP = ctx->allocator->make<Parameter>();
          newP->argIndex = p->index;
          newP->value = ctx->allocator->make<PapyrusLiteralExpression>(location, p->defaultValue);
          newArgs.insertBefore(newArgsIter, newP);
        } else {
          ++newArgsIter;
        }
      }
      arguments = std::move(newArgs);
    }

    for (auto p : arguments.lockstepIterate(function.func->parameters)) {
      p.self->value->semantic(ctx);
      ctx->checkForPoison(p.self->value);
      switch (p.other->type.type) {
        case PapyrusType::Kind::CustomEventName:
        case PapyrusType::Kind::ScriptEventName:
        {
          bool isCustomEvent = p.other->type.type == PapyrusType::Kind::CustomEventName;

          auto le = p.self->value->as<expressions::PapyrusLiteralExpression>();
          if (!le || le->value.type != PapyrusValueType::String) {
            ctx->reportingContext.error(p.self->value->location, "Argument %zu must be string literal.", p.other->index);
            continue;
          }

          auto baseType = [&]() -> PapyrusType {
            if (p.other->index != 0)
              return p.prevSelf->value->resultType();
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
          ctx->reportingContext.error(p.self->value->location, "Unable to resolve %s event named '%s' in '%s' or one of its parents.", isCustomEvent ? "a custom" : "an", le->value.s.to_string().c_str(), baseType.prettyString().c_str());
          break;
        }
        default:
          break;
      }
    }

    // We need the semantic pass to have run for the args, but we can't have them coerced until after
    // we've transformed CustomEventName and ScriptEventName parameters.
    for (auto p : arguments.lockstepIterate(function.func->parameters)) {
      if (p.other->type.type == PapyrusType::Kind::CustomEventName) {
        p.self->value = ctx->coerceExpression(p.self->value, PapyrusType::String(p.self->value->location));
      } else if (p.other->type.type == PapyrusType::Kind::ScriptEventName) {
        p.self->value = ctx->coerceExpression(p.self->value, PapyrusType::String(p.self->value->location));
      } else {
        p.self->value = ctx->coerceExpression(p.self->value, p.other->type);
      }
    }

    if (function.func->name == "GotoState" && arguments.size() == 1) {
      auto le = arguments.front()->value->as<PapyrusLiteralExpression>();
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
