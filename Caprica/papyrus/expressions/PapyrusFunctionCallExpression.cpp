#include <papyrus/expressions/PapyrusFunctionCallExpression.h>

#include <cstring>

#include <boost/algorithm/string/case_conv.hpp>

#include <papyrus/PapyrusFunction.h>
#include <papyrus/PapyrusObject.h>
#include <papyrus/expressions/PapyrusLiteralExpression.h>
#include <papyrus/expressions/PapyrusParentExpression.h>

namespace caprica { namespace papyrus { namespace expressions {

pex::PexValue PapyrusFunctionCallExpression::generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr, PapyrusExpression* base) const {
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
      default:
        CapricaError::logicalFatal("Unknown PapyrusBuiltinArrayFunctionKind!");
    }
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
      args.push_back(param->value->generateLoad(file, bldr));
    bldr << location;
    if (function.func->isGlobal()) {
      std::string objName = function.func->parentObject->name;
      boost::algorithm::to_lower(objName);
      bldr << op::callstatic{ file->getString(objName), file->getString(function.func->name), dest, args };
    } else if (base && base->is<PapyrusParentExpression>()) {
      bldr << op::callparent{ file->getString(function.func->name), dest, args };
    } else if (base) {
      auto bVal = base->generateLoad(file, bldr);
      bldr << op::callmethod{ file->getString(function.func->name), bVal, dest, args };
    } else {
      bldr << op::callmethod{ file->getString(function.func->name), pex::PexValue::Identifier(file->getString("self")), dest, args };
    }

    if (function.func->returnType.type == PapyrusType::Kind::None)
      return pex::PexValue::Invalid();
    return dest;
  }
}

void PapyrusFunctionCallExpression::semantic(PapyrusResolutionContext* ctx) {
  function = ctx->resolveFunctionIdentifier(PapyrusType::None(location), function);

  if (function.type == PapyrusIdentifierType::BuiltinArrayFunction) {
    for (size_t i = 0; i < arguments.size(); i++)
      arguments[i]->value->semantic(ctx);

    switch (function.arrayFuncKind) {
      case PapyrusBuiltinArrayFunctionKind::Find:
      {
        if (arguments.size() < 1 || arguments.size() > 2)
          CapricaError::fatal(location, "Expected either 1 or 2 parameters to 'Find'!");
        arguments[0]->value = PapyrusResolutionContext::coerceExpression(arguments[0]->value, *function.arrayFuncElementType);
        if (arguments.size() == 1) {
          auto p = new Parameter();
          p->value = new PapyrusLiteralExpression(location, PapyrusValue::Integer(location, 0));
          arguments.push_back(p);
        } else {
          if (arguments[1]->name != "" && _stricmp(arguments[1]->name.c_str(), "aiStartIndex"))
            CapricaError::error(arguments[1]->value->location, "Unknown argument '%s'! Was expecting 'aiStartIndex'!", arguments[1]->name.c_str());
          arguments[1]->value = PapyrusResolutionContext::coerceExpression(arguments[1]->value, PapyrusType::Int(arguments[1]->value->location));
        }
        break;
      }
      case PapyrusBuiltinArrayFunctionKind::FindStruct:
      {
        if (arguments.size() < 2 || arguments.size() > 3)
          CapricaError::fatal(location, "Expected either 2 or 3 parameters to 'FindStruct'!");
        if (arguments[0]->value->resultType().type != PapyrusType::Kind::String)
          CapricaError::fatal(location, "Expected the literal name of the struct member as a string to compare against!");
        
        std::string memberName = arguments[0]->value->as<PapyrusLiteralExpression>()->value.s;
        PapyrusType elemType = PapyrusType::Default();
        for (auto m : function.arrayFuncElementType->resolvedStruct->members) {
          if (!_stricmp(m->name.c_str(), memberName.c_str())) {
            elemType = m->type;
            break;
          }
        }
        if (elemType.type == PapyrusType::Kind::None)
          CapricaError::fatal(arguments[0]->value->location, "Unknown member '%s' of struct '%s'!", memberName.c_str(), function.arrayFuncElementType->resolvedStruct->name.c_str());
        arguments[1]->value = PapyrusResolutionContext::coerceExpression(arguments[1]->value, elemType);

        if (arguments.size() == 2) {
          auto p = new Parameter();
          p->value = new PapyrusLiteralExpression(location, PapyrusValue::Integer(location, 0));
          arguments.push_back(p);
        } else {
          if (arguments[2]->name != "" && _stricmp(arguments[2]->name.c_str(), "aiStartIndex"))
            CapricaError::error(arguments[2]->value->location, "Unknown argument '%s'! Was expecting 'aiStartIndex'!", arguments[2]->name.c_str());
          arguments[2]->value = PapyrusResolutionContext::coerceExpression(arguments[2]->value, PapyrusType::Int(arguments[2]->value->location));
        }
        break;
      }
      case PapyrusBuiltinArrayFunctionKind::RFind:
      {
        if (arguments.size() < 1 || arguments.size() > 2)
          CapricaError::fatal(location, "Expected either 1 or 2 parameters to 'RFind'!");
        arguments[0]->value = PapyrusResolutionContext::coerceExpression(arguments[0]->value, *function.arrayFuncElementType);
        if (arguments.size() == 1) {
          auto p = new Parameter();
          p->value = new PapyrusLiteralExpression(location, PapyrusValue::Integer(location, -1));
          arguments.push_back(p);
        } else {
          if (arguments[1]->name != "" && _stricmp(arguments[1]->name.c_str(), "aiStartIndex"))
            CapricaError::error(arguments[1]->value->location, "Unknown argument '%s'! Was expecting 'aiStartIndex'!", arguments[1]->name.c_str());
          arguments[1]->value = PapyrusResolutionContext::coerceExpression(arguments[1]->value, PapyrusType::Int(arguments[1]->value->location));
        }
        break;
      }
      case PapyrusBuiltinArrayFunctionKind::RFindStruct:
      {
        if (arguments.size() < 2 || arguments.size() > 3)
          CapricaError::fatal(location, "Expected either 2 or 3 parameters to 'RFindStruct'!");
        if (arguments[0]->value->resultType().type != PapyrusType::Kind::String)
          CapricaError::fatal(location, "Expected the literal name of the struct member as a string to compare against!");

        std::string memberName = arguments[0]->value->as<PapyrusLiteralExpression>()->value.s;
        PapyrusType elemType = PapyrusType::Default();
        for (auto m : function.arrayFuncElementType->resolvedStruct->members) {
          if (!_stricmp(m->name.c_str(), memberName.c_str())) {
            elemType = m->type;
            break;
          }
        }
        if (elemType.type == PapyrusType::Kind::None)
          CapricaError::fatal(arguments[0]->value->location, "Unknown member '%s' of struct '%s'!", memberName.c_str(), function.arrayFuncElementType->resolvedStruct->name.c_str());
        arguments[1]->value = PapyrusResolutionContext::coerceExpression(arguments[1]->value, elemType);

        if (arguments.size() == 2) {
          auto p = new Parameter();
          p->value = new PapyrusLiteralExpression(location, PapyrusValue::Integer(location, -1));
          arguments.push_back(p);
        } else {
          if (arguments[2]->name != "" && _stricmp(arguments[2]->name.c_str(), "aiStartIndex"))
            CapricaError::error(arguments[2]->value->location, "Unknown argument '%s'! Was expecting 'aiStartIndex'!", arguments[2]->name.c_str());
          arguments[2]->value = PapyrusResolutionContext::coerceExpression(arguments[2]->value, PapyrusType::Int(arguments[2]->value->location));
        }
        break;
      }
      case PapyrusBuiltinArrayFunctionKind::Add:
      {
        if (arguments.size() < 1 || arguments.size() > 2)
          CapricaError::fatal(location, "Expected either 1 or 2 parameters to 'Add'!");
        arguments[0]->value = PapyrusResolutionContext::coerceExpression(arguments[0]->value, *function.arrayFuncElementType);
        if (arguments.size() == 1) {
          auto p = new Parameter();
          p->value = new PapyrusLiteralExpression(location, PapyrusValue::Integer(location, 1));
          arguments.push_back(p);
        } else {
          if (arguments[1]->name != "" && _stricmp(arguments[1]->name.c_str(), "aiCount"))
            CapricaError::error(arguments[1]->value->location, "Unknown argument '%s'! Was expecting 'aiCount'!", arguments[1]->name.c_str());
          arguments[1]->value = PapyrusResolutionContext::coerceExpression(arguments[1]->value, PapyrusType::Int(arguments[1]->value->location));
        }
        break;
      }
      case PapyrusBuiltinArrayFunctionKind::Clear:
        if (arguments.size() != 0)
          CapricaError::fatal(location, "Expected 0 parameters to 'Clear'!");
        break;
      case PapyrusBuiltinArrayFunctionKind::Insert:
        if (arguments.size() != 2)
          CapricaError::fatal(location, "Expected 2 parameters to 'Insert'!");
        arguments[0]->value = PapyrusResolutionContext::coerceExpression(arguments[0]->value, *function.arrayFuncElementType);
        arguments[1]->value = PapyrusResolutionContext::coerceExpression(arguments[1]->value, PapyrusType::Int(arguments[1]->value->location));
        break;
      case PapyrusBuiltinArrayFunctionKind::Remove:
      {
        if (arguments.size() < 1 || arguments.size() > 2)
          CapricaError::fatal(location, "Expected either 1 or 2 parameters to 'Remove'!");
        arguments[0]->value = PapyrusResolutionContext::coerceExpression(arguments[0]->value, PapyrusType::Int(arguments[0]->value->location));
        if (arguments.size() == 1) {
          auto p = new Parameter();
          p->value = new PapyrusLiteralExpression(location, PapyrusValue::Integer(location, 1));
          arguments.push_back(p);
        } else {
          if (arguments[1]->name != "" && _stricmp(arguments[1]->name.c_str(), "aiCount"))
            CapricaError::error(arguments[1]->value->location, "Unknown argument '%s'! Was expecting 'aiCount'!", arguments[1]->name.c_str());
          arguments[1]->value = PapyrusResolutionContext::coerceExpression(arguments[1]->value, PapyrusType::Int(arguments[1]->value->location));
        }
        break;
      }
      case PapyrusBuiltinArrayFunctionKind::RemoveLast:
        if (arguments.size() != 0)
          CapricaError::fatal(location, "Expected 0 parameters to 'RemoveLast'!");
        break;
      default:
        CapricaError::logicalFatal("Unknown PapyrusBuiltinArrayFunctionKind!");
    }
  } else {
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
            if (!_stricmp(function.func->parameters[i2]->name.c_str(), arguments[i]->name.c_str())) {
              baseI = i2;
              goto ContinueOuterLoop;
            }
          }
          CapricaError::fatal(arguments[i]->value->location, "Unable to find a parameter named '%s'!", arguments[i]->name.c_str());
        }
        if (hadNamedArgs)
          CapricaError::fatal(arguments[i]->value->location, "No normal arguments are allowed after the first named argument!");
      ContinueOuterLoop:
        newArgs[baseI] = arguments[i];
      }

      for (size_t i = 0; i < newArgs.size(); i++) {
        if (newArgs[i] == nullptr) {
          if (function.func->parameters[i]->defaultValue.type == PapyrusValueType::Invalid)
            CapricaError::fatal(location, "Not enough arguments provided.");
          newArgs[i] = new Parameter();
          newArgs[i]->value = new PapyrusLiteralExpression(location, function.func->parameters[i]->defaultValue);
        }
      }
      arguments = newArgs;
    }

    for (size_t i = 0; i < arguments.size(); i++) {
      arguments[i]->value->semantic(ctx);
      arguments[i]->value = PapyrusResolutionContext::coerceExpression(arguments[i]->value, function.func->parameters[i]->type);
    }

    if (function.func->name == "GotoState" && arguments.size() == 1) {
      auto le = arguments[0]->value->as<PapyrusLiteralExpression>();
      if (le && le->value.type == PapyrusValueType::String) {
        auto targetStateName = le->value.s;
        if (!ctx->tryResolveState(targetStateName))
          CapricaError::Warning::W4003_State_Doesnt_Exist(le->location, targetStateName.c_str());
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
    return function.func->returnType;
  }
}

}}}
