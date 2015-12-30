#pragma once

#include <cstring>
#include <string>
#include <vector>

#include <papyrus/PapyrusFunction.h>
#include <papyrus/PapyrusIdentifier.h>
#include <papyrus/PapyrusObject.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/expressions/PapyrusLiteralExpression.h>
#include <papyrus/expressions/PapyrusParentExpression.h>
#include <papyrus/parser/PapyrusFileLocation.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusFunctionCallExpression final : public PapyrusExpression
{
  struct Parameter final
  {
    std::string name{ "" };
    PapyrusExpression* value{ nullptr };

    Parameter() = default;
    ~Parameter() {
      if (value)
        delete value;
    }
  };
  PapyrusIdentifier function{ };
  std::vector<Parameter*> arguments{ };

  PapyrusFunctionCallExpression(parser::PapyrusFileLocation loc) : PapyrusExpression(loc) { }
  ~PapyrusFunctionCallExpression() = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    return generateLoad(file, bldr, nullptr);
  }

  pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr, PapyrusExpression* base) const {
    namespace op = caprica::pex::op;
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
      bldr << op::callstatic{ file->getString(function.func->parentObject->name), file->getString(function.func->name), dest, args };
    } else if (base && base->is<PapyrusParentExpression>()) {
      bldr << op::callparent{ file->getString(function.func->name), dest, args };
    } else if (base) {
      auto bVal = base->generateLoad(file, bldr);
      bldr << op::callmethod{ file->getString(function.func->name), bVal, dest, args };
      bldr.freeIfTemp(bVal);
    } else {
      bldr << op::callmethod{ file->getString(function.func->name), pex::PexValue::Identifier(file->getString("self")), dest, args };
    }
    return dest;
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    function = ctx->resolveFunctionIdentifier(PapyrusType::None(), function);

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

  virtual PapyrusType resultType() const override {
    return function.func->returnType;
  }
};

}}}
