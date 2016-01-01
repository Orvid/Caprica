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
        auto dest = bldr.allocTemp(file, resultType());
        bldr << location;
        bldr << op::arrayfindelement{ bVal, dest, elem, idx };
        bldr.freeIfTemp(bVal);
        bldr.freeIfTemp(elem);
        bldr.freeIfTemp(idx);
        return dest;
      }
      case PapyrusBuiltinArrayFunctionKind::FindStruct:
      {
        auto memberName = arguments[0]->value->as<PapyrusLiteralExpression>()->value.buildPex(file);
        auto elem = arguments[1]->value->generateLoad(file, bldr);
        auto idx = arguments[2]->value->generateLoad(file, bldr);
        auto dest = bldr.allocTemp(file, resultType());
        bldr << location;
        bldr << op::arrayfindstruct{ bVal, dest, memberName, elem, idx };
        bldr.freeIfTemp(bVal);
        bldr.freeIfTemp(elem);
        bldr.freeIfTemp(idx);
        return dest;
      }
      case PapyrusBuiltinArrayFunctionKind::RFind:
      {
        auto elem = arguments[0]->value->generateLoad(file, bldr);
        auto idx = arguments[1]->value->generateLoad(file, bldr);
        auto dest = bldr.allocTemp(file, resultType());
        bldr << location;
        bldr << op::arrayrfindelement{ bVal, dest, elem, idx };
        bldr.freeIfTemp(bVal);
        bldr.freeIfTemp(elem);
        bldr.freeIfTemp(idx);
        return dest;
      }
      case PapyrusBuiltinArrayFunctionKind::RFindStruct:
      {
        auto memberName = arguments[0]->value->as<PapyrusLiteralExpression>()->value.buildPex(file);
        auto elem = arguments[1]->value->generateLoad(file, bldr);
        auto idx = arguments[2]->value->generateLoad(file, bldr);
        auto dest = bldr.allocTemp(file, resultType());
        bldr << location;
        bldr << op::arrayrfindstruct{ bVal, dest, memberName, elem, idx };
        bldr.freeIfTemp(bVal);
        bldr.freeIfTemp(elem);
        bldr.freeIfTemp(idx);
        return dest;
      }
      case PapyrusBuiltinArrayFunctionKind::Add:
      {
        auto elem = arguments[0]->value->generateLoad(file, bldr);
        auto cnt = arguments[1]->value->generateLoad(file, bldr);
        auto dest = bldr.allocTemp(file, resultType());
        bldr << location;
        bldr << op::arrayadd{ bVal, elem, cnt };
        bldr.freeIfTemp(bVal);
        bldr.freeIfTemp(elem);
        bldr.freeIfTemp(cnt);
        return pex::PexValue::Invalid();
      }
      case PapyrusBuiltinArrayFunctionKind::Clear:
      {
          bldr << location;
          bldr << op::arrayclear{ bVal };
          bldr.freeIfTemp(bVal);
          return pex::PexValue::Invalid();
      }
      case PapyrusBuiltinArrayFunctionKind::Insert:
      {
        auto elem = arguments[0]->value->generateLoad(file, bldr);
        auto idx = arguments[1]->value->generateLoad(file, bldr);
        auto dest = bldr.allocTemp(file, resultType());
        bldr << location;
        bldr << op::arrayinsert{ bVal, elem, idx };
        bldr.freeIfTemp(bVal);
        bldr.freeIfTemp(elem);
        bldr.freeIfTemp(idx);
        return pex::PexValue::Invalid();
      }
      case PapyrusBuiltinArrayFunctionKind::Remove:
      {
        auto idx = arguments[0]->value->generateLoad(file, bldr);
        auto cnt = arguments[1]->value->generateLoad(file, bldr);
        auto dest = bldr.allocTemp(file, resultType());
        bldr << location;
        bldr << op::arrayremove{ bVal, idx, cnt };
        bldr.freeIfTemp(bVal);
        bldr.freeIfTemp(idx);
        bldr.freeIfTemp(cnt);
        return pex::PexValue::Invalid();
      }
      case PapyrusBuiltinArrayFunctionKind::RemoveLast:
      {
        bldr << location;
        bldr << op::arrayremovelast{ bVal };
        bldr.freeIfTemp(bVal);
        return pex::PexValue::Invalid();
      }
      default:
        throw std::runtime_error("Unknown PapyrusBuiltinArrayFunctionKind!");
    }
  } else {
    pex::PexLocalVariable* dest;
    if (function.func->returnType == PapyrusType::None())
      dest = bldr.getNoneLocal(file);
    else
      dest = bldr.allocTemp(file, resultType());

    std::vector<pex::PexValue> args;
    for (auto param : arguments)
      args.push_back(param->value->generateLoad(file, bldr));
    bldr << location;
    if (function.func->isGlobal) {
      std::string objName = function.func->parentObject->name;
      boost::algorithm::to_lower(objName);
      bldr << op::callstatic{ file->getString(objName), file->getString(function.func->name), dest, args };
    } else if (base && base->is<PapyrusParentExpression>()) {
      bldr << op::callparent{ file->getString(function.func->name), dest, args };
    } else if (base) {
      auto bVal = base->generateLoad(file, bldr);
      bldr << op::callmethod{ file->getString(function.func->name), bVal, dest, args };
      bldr.freeIfTemp(bVal);
    } else {
      bldr << op::callmethod{ file->getString(function.func->name), pex::PexValue::Identifier(file->getString("self")), dest, args };
    }

    if (file->getStringValue(dest->type) == "None")
      return pex::PexValue::Invalid();
    return dest;
  }
}

void PapyrusFunctionCallExpression::semantic(PapyrusResolutionContext* ctx) {
  function = ctx->resolveFunctionIdentifier(PapyrusType::None(), function);

  if (function.type == PapyrusIdentifierType::BuiltinArrayFunction) {
    for (size_t i = 0; i < arguments.size(); i++)
      arguments[i]->value->semantic(ctx);

    switch (function.arrayFuncKind) {
      NormalFind:
      case PapyrusBuiltinArrayFunctionKind::Find:
      {
        if (arguments.size() < 1 || arguments.size() > 2)
          ctx->fatalError("Expected either 1 or 2 parameters to 'find'!");
        arguments[0]->value = coerceExpression(arguments[0]->value, function.arrayFuncElementType);
        if (arguments.size() == 1) {
          auto p = new Parameter();
          auto le = new PapyrusLiteralExpression(location);
          le->value.type = PapyrusValueType::Integer;
          le->value.i = 0;
          p->value = le;
          arguments.push_back(p);
        } else {
          if (arguments[1]->name != "" && _stricmp(arguments[1]->name.c_str(), "aiStartIndex"))
            ctx->fatalError("Unknown argument '" + arguments[1]->name + "'! Was expecting 'aiStartIndex'!");
          arguments[1]->value = coerceExpression(arguments[1]->value, PapyrusType::Int());
        }
        break;
      }
      case PapyrusBuiltinArrayFunctionKind::FindStruct:
      {
        if (arguments.size() == 1 || !arguments[0]->value->is<PapyrusLiteralExpression>()) {
          function.arrayFuncKind = PapyrusBuiltinArrayFunctionKind::Find;
          goto NormalFind;
        }
        if (arguments.size() < 2 || arguments.size() > 3)
          ctx->fatalError("Expected either 2 or 3 parameters to 'find'!");
        if (arguments[0]->value->resultType() != PapyrusType::String())
          ctx->fatalError("Expected the literal name of the struct member as a string to compare against!");
        
        std::string memberName = arguments[0]->value->as<PapyrusLiteralExpression>()->value.s;
        PapyrusType elemType;
        for (auto m : function.arrayFuncElementType.resolvedStruct->members) {
          if (!_stricmp(m->name.c_str(), memberName.c_str())) {
            elemType = m->type;
            break;
          }
        }
        if (elemType == PapyrusType::None())
          ctx->fatalError("Unknown member '" + memberName + "' of struct '" + function.arrayFuncElementType.resolvedStruct->name + "'!");
        arguments[1]->value = coerceExpression(arguments[1]->value, elemType);

        if (arguments.size() == 2) {
          auto p = new Parameter();
          auto le = new PapyrusLiteralExpression(location);
          le->value.type = PapyrusValueType::Integer;
          le->value.i = 0;
          p->value = le;
          arguments.push_back(p);
        } else {
          if (arguments[2]->name != "" && _stricmp(arguments[2]->name.c_str(), "aiStartIndex"))
            ctx->fatalError("Unknown argument '" + arguments[2]->name + "'! Was expecting 'aiStartIndex'!");
          arguments[2]->value = coerceExpression(arguments[2]->value, PapyrusType::Int());
        }
        break;
      }
      NormalRFind:
      case PapyrusBuiltinArrayFunctionKind::RFind:
      {
        if (arguments.size() < 1 || arguments.size() > 2)
          ctx->fatalError("Expected either 1 or 2 parameters to 'rfind'!");
        arguments[0]->value = coerceExpression(arguments[0]->value, function.arrayFuncElementType);
        if (arguments.size() == 1) {
          auto p = new Parameter();
          auto le = new PapyrusLiteralExpression(location);
          le->value.type = PapyrusValueType::Integer;
          le->value.i = -1;
          p->value = le;
          arguments.push_back(p);
        } else {
          if (arguments[1]->name != "" && _stricmp(arguments[1]->name.c_str(), "aiStartIndex"))
            ctx->fatalError("Unknown argument '" + arguments[1]->name + "'! Was expecting 'aiStartIndex'!");
          arguments[1]->value = coerceExpression(arguments[1]->value, PapyrusType::Int());
        }
        break;
      }
      case PapyrusBuiltinArrayFunctionKind::RFindStruct:
      {
        if (arguments.size() == 1 || !arguments[0]->value->is<PapyrusLiteralExpression>()) {
          function.arrayFuncKind = PapyrusBuiltinArrayFunctionKind::RFind;
          goto NormalRFind;
        }
        if (arguments.size() < 2 || arguments.size() > 3)
          ctx->fatalError("Expected either 2 or 3 parameters to 'rfind'!");
        if (arguments[0]->value->resultType() != PapyrusType::String())
          ctx->fatalError("Expected the literal name of the struct member as a string to compare against!");

        std::string memberName = arguments[0]->value->as<PapyrusLiteralExpression>()->value.s;
        PapyrusType elemType;
        for (auto m : function.arrayFuncElementType.resolvedStruct->members) {
          if (!_stricmp(m->name.c_str(), memberName.c_str())) {
            elemType = m->type;
            break;
          }
        }
        if (elemType == PapyrusType::None())
          ctx->fatalError("Unknown member '" + memberName + "' of struct '" + function.arrayFuncElementType.resolvedStruct->name + "'!");
        arguments[1]->value = coerceExpression(arguments[1]->value, elemType);

        if (arguments.size() == 2) {
          auto p = new Parameter();
          auto le = new PapyrusLiteralExpression(location);
          le->value.type = PapyrusValueType::Integer;
          le->value.i = -1;
          p->value = le;
          arguments.push_back(p);
        } else {
          if (arguments[2]->name != "" && _stricmp(arguments[2]->name.c_str(), "aiStartIndex"))
            ctx->fatalError("Unknown argument '" + arguments[2]->name + "'! Was expecting 'aiStartIndex'!");
          arguments[2]->value = coerceExpression(arguments[2]->value, PapyrusType::Int());
        }
        break;
      }
      case PapyrusBuiltinArrayFunctionKind::Add:
      {
        if (arguments.size() < 1 || arguments.size() > 2)
          ctx->fatalError("Expected either 1 or 2 parameters to 'add'!");
        arguments[0]->value = coerceExpression(arguments[0]->value, function.arrayFuncElementType);
        if (arguments.size() == 1) {
          auto p = new Parameter();
          auto le = new PapyrusLiteralExpression(location);
          le->value.type = PapyrusValueType::Integer;
          le->value.i = 1;
          p->value = le;
          arguments.push_back(p);
        } else {
          if (arguments[1]->name != "" && _stricmp(arguments[1]->name.c_str(), "aiCount"))
            ctx->fatalError("Unknown argument '" + arguments[1]->name + "'! Was expecting 'aiCount'!");
          arguments[1]->value = coerceExpression(arguments[1]->value, PapyrusType::Int());
        }
        break;
      }
      case PapyrusBuiltinArrayFunctionKind::Clear:
        if (arguments.size() != 0)
          ctx->fatalError("Expected 0 parameters to 'clear'!");
        break;
      case PapyrusBuiltinArrayFunctionKind::Insert:
        if (arguments.size() != 2)
          ctx->fatalError("Expected 2 parameters to 'insert'!");
        arguments[0]->value = coerceExpression(arguments[0]->value, function.arrayFuncElementType);
        arguments[1]->value = coerceExpression(arguments[1]->value, PapyrusType::Int());
        break;
      case PapyrusBuiltinArrayFunctionKind::Remove:
      {
        if (arguments.size() < 1 || arguments.size() > 2)
          ctx->fatalError("Expected either 1 or 2 parameters to 'remove'!");
        arguments[0]->value = coerceExpression(arguments[0]->value, function.arrayFuncElementType);
        if (arguments.size() == 1) {
          auto p = new Parameter();
          auto le = new PapyrusLiteralExpression(location);
          le->value.type = PapyrusValueType::Integer;
          le->value.i = 1;
          p->value = le;
          arguments.push_back(p);
        } else {
          if (arguments[1]->name != "" && _stricmp(arguments[1]->name.c_str(), "aiCount"))
            ctx->fatalError("Unknown argument '" + arguments[1]->name + "'! Was expecting 'aiCount'!");
          arguments[1]->value = coerceExpression(arguments[1]->value, PapyrusType::Int());
        }
        break;
      }
      case PapyrusBuiltinArrayFunctionKind::RemoveLast:
        if (arguments.size() != 0)
          ctx->fatalError("Expected 0 parameters to 'removelast'!");
        break;
      default:
        throw std::runtime_error("Unknown PapyrusBuiltinArrayFunctionKind!");
    }
  } else {
    if (arguments.size() != function.func->parameters.size()) {
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
          ctx->fatalError("Unable to find a parameter named '" + arguments[i]->name + "'!");
        }
        if (hadNamedArgs)
          ctx->fatalError("No normal arguments are allowed after the first named argument!");
      ContinueOuterLoop:
        newArgs[baseI] = arguments[i];
      }

      for (size_t i = 0; i < newArgs.size(); i++) {
        if (newArgs[i] == nullptr) {
          if (!function.func->parameters[i]->hasDefaultValue)
            ctx->fatalError("Not enough arguments provided.");
          newArgs[i] = new Parameter();
          auto val = new PapyrusLiteralExpression(location);
          val->value = function.func->parameters[i]->defaultValue;
          newArgs[i]->value = val;
        }
      }
      arguments = newArgs;
    }

    for (size_t i = 0; i < arguments.size(); i++) {
      arguments[i]->value->semantic(ctx);
      arguments[i]->value = coerceExpression(arguments[i]->value, function.func->parameters[i]->type);
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
        return PapyrusType::Int();
      default:
        return PapyrusType::None();
    }
  } else {
    return function.func->returnType;
  }
}

}}}
