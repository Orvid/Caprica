#include <papyrus/parser/PapyrusParser.h>

#include <papyrus/PapyrusObject.h>

namespace caprica {
namespace papyrus {
namespace parser {

PapyrusScript* PapyrusParser::parseScript() {
  auto script = new PapyrusScript();
  script->objects.push_back(parseObject(script));
  return script;
}

PapyrusObject* PapyrusParser::parseObject(PapyrusScript* script) {
  auto obj = new PapyrusObject();

  maybeConsumeEOLs();
  // FO4Extra: Const.
  if (maybeConsume(TokenType::kConst))
    obj->isConst = true;
  expectConsume(TokenType::kScriptName);
  // TODO: This is supposidly supposed to match the name of the file, should check.
  obj->name = expectConsumeIdent();
  if (maybeConsume(TokenType::kExtends))
    obj->parentClass = PapyrusType::Unresolved(expectConsumeIdent());
  obj->userFlags = parseUserFlags(PapyrusUserFlags::Hidden | PapyrusUserFlags::Conditional);
  expectConsumeEOLs();
  obj->documentationString = maybeConsumeDocString();

  while (cur.type != TokenType::END) {
    bool isConst = false;
    switch (cur.type) {
      case TokenType::kImport:
        consume();
        obj->imports.push_back(expectConsumeIdent());
        expectConsume(TokenType::EOL);
        break;

      case TokenType::kAuto:
        consume();
        expectConsume(TokenType::kState);
        obj->states.push_back(parseState(script, obj, true));
        break;
      case TokenType::kState:
        consume();
        obj->states.push_back(parseState(script, obj, false));
        break;

      case TokenType::kStruct:
        consume();
        obj->structs.push_back(parseStruct(script, obj));
        break;

      case TokenType::kPropertyGroup:
        consume();
        obj->propertyGroups.push_back(parsePropertyGroup(script, obj));
        break;

      case TokenType::kEvent:
        consume();
        obj->getRootState()->functions.push_back(parseFunction(script, obj, obj->getRootState(), PapyrusType::None(), TokenType::kEndEvent));
        break;
      case TokenType::kFunction:
        consume();
        obj->getRootState()->functions.push_back(parseFunction(script, obj, obj->getRootState(), PapyrusType::None(), TokenType::kEndFunction));
        break;

      case TokenType::kConst:
        consume();
        isConst = true;
        goto TypeValue;

      TypeValue:
      case TokenType::kBool:
      case TokenType::kFloat:
      case TokenType::kInt:
      case TokenType::kString:
      case TokenType::kVar:
      case TokenType::Identifier:
      {
        auto tp = expectConsumePapyrusType();
        if (cur.type == TokenType::kFunction) {
          if (isConst)
            fatalError("The return type of a function cannot be marked const!");
          consume();
          obj->getRootState()->functions.push_back(parseFunction(script, obj, obj->getRootState(), tp, TokenType::kEndFunction));
        } else if (cur.type == TokenType::kProperty) {
          consume();
          obj->getRootPropertyGroup()->properties.push_back(parseProperty(script, obj, isConst, tp));
        } else {
          obj->variables.push_back(parseVariable(script, obj, isConst, tp));
        }
        break;
      }

      default:
        script->sourceFileName += cur.asString() + "\r\n";
        consume();
        break;
    }
  }

  return obj;
}

PapyrusState* PapyrusParser::parseState(PapyrusScript* script, PapyrusObject* object, bool isAuto) {
  auto state = new PapyrusState();
  state->name = expectConsumeIdent();
  if (isAuto) {
    if (object->autoState != nullptr)
      fatalError("Only one state can be declared auto. '" + object->autoState->name + "' was already declared as the auto state.");
    object->autoState = state;
  }
  expectConsumeEOLs();

  while (true) {
    switch (cur.type) {
      case TokenType::kEndState:
        consume();
        expectConsumeEOLs();
        goto Return;

      case TokenType::kEvent:
        consume();
        state->functions.push_back(parseFunction(script, object, state, PapyrusType::None(), TokenType::kEndEvent));
        break;
      case TokenType::kFunction:
        consume();
        state->functions.push_back(parseFunction(script, object, state, PapyrusType::None(), TokenType::kEndFunction));
        break;

      case TokenType::kBool:
      case TokenType::kFloat:
      case TokenType::kInt:
      case TokenType::kString:
      case TokenType::kVar:
      case TokenType::Identifier:
      {
        auto tp = expectConsumePapyrusType();
        expectConsume(TokenType::kFunction);
        state->functions.push_back(parseFunction(script, object, state, tp, TokenType::kEndFunction));
        break;
      }

      default:
        fatalError("Expected an event or function!");
    }
  }

Return:
  return state;
}

PapyrusStruct* PapyrusParser::parseStruct(PapyrusScript* script, PapyrusObject* object) {
  auto struc = new PapyrusStruct();
  struc->name = expectConsumeIdent();
  expectConsumeEOLs();

  while (true) {
    bool isConst = false;
    switch (cur.type) {
      case TokenType::kEndStruct:
        consume();
        expectConsumeEOLs();
        goto Return;

      case TokenType::kConst:
        consume();
        isConst = true;
        goto TypeValue;

      TypeValue:
      case TokenType::kBool:
      case TokenType::kFloat:
      case TokenType::kInt:
      case TokenType::kString:
      case TokenType::kVar:
      case TokenType::Identifier:
      {
        auto tp = expectConsumePapyrusType();
        struc->members.push_back(parseStructMember(script, object, struc, isConst, tp));
        break;
      }

      default:
        fatalError("Unexpected token while parsing struct!");
    }
  }

Return:
  return struc;
}

PapyrusStructMember* PapyrusParser::parseStructMember(PapyrusScript* script, PapyrusObject* object, PapyrusStruct* struc, bool isConst, PapyrusType tp) {
  auto mem = new PapyrusStructMember();
  mem->isConst = isConst;
  mem->type = tp;
  mem->name = expectConsumeIdent();

  if (maybeConsume(TokenType::Equal)) {
    mem->defaultValue = expectConsumePapyrusValue();
  } else if (isConst) {
    fatalError("A constant variable must have a value!");
  }

  mem->userFlags = parseUserFlags(PapyrusUserFlags::Conditional);
  expectConsumeEOLs();
  mem->documentationString = maybeConsumeDocString();
  return mem;
}

PapyrusPropertyGroup* PapyrusParser::parsePropertyGroup(PapyrusScript* script, PapyrusObject* object) {
  auto group = new PapyrusPropertyGroup();
  group->name = expectConsumeIdent();
  group->userFlags = parseUserFlags(PapyrusUserFlags::Hidden);
  expectConsumeEOLs();
  group->documentationComment = maybeConsumeDocString();

  while (true) {
    bool isConst = false;
    switch (cur.type) {
      case TokenType::kEndPropertyGroup:
        consume();
        expectConsumeEOLs();
        goto Return;

      case TokenType::kConst:
        consume();
        isConst = true;
        goto TypeValue;

      TypeValue:
      case TokenType::kBool:
      case TokenType::kFloat:
      case TokenType::kInt:
      case TokenType::kString:
      case TokenType::kVar:
      case TokenType::Identifier:
      {
        auto tp = expectConsumePapyrusType();
        expectConsume(TokenType::kProperty);
        group->properties.push_back(parseProperty(script, object, isConst, tp));
        break;
      }

      default:
        fatalError("Unexpected token while parsing property group!");
    }
  }

Return:
  return group;
}

PapyrusFunction* PapyrusParser::parseFunction(PapyrusScript* script, PapyrusObject* object, PapyrusState* state, PapyrusType returnType, TokenType endToken) {
  auto func = new PapyrusFunction();
  func->name = expectConsumeIdent();
  func->returnType = returnType;
  expectConsume(TokenType::LParen);

  if (cur.type != TokenType::RParen) {
    do {
      maybeConsume(TokenType::Comma);

      auto param = new PapyrusFunctionParameter();
      param->type = expectConsumePapyrusType();
      param->name = expectConsumeIdent();
      if (maybeConsume(TokenType::Equal))
        param->defaultValue = expectConsumePapyrusValue();
      func->parameters.push_back(param);
    } while (cur.type == TokenType::Comma);
  }
  expectConsume(TokenType::RParen);

  if (endToken == TokenType::kEndFunction && maybeConsume(TokenType::kGlobal))
    func->isGlobal = true;
  if (maybeConsume(TokenType::kNative))
    func->isNative = true;
  if (endToken == TokenType::kEndFunction && maybeConsume(TokenType::kGlobal)) {
    if (func->isGlobal)
      fatalError("This function was already declared global!");
    func->isGlobal = true;
  }

  func->userFlags = parseUserFlags(PapyrusUserFlags::None);
  expectConsumeEOLs();
  func->documentationComment = maybeConsumeDocString();
  if (!func->isNative) {
    while (cur.type != endToken && cur.type != TokenType::END) {
      consume();
      // Here we parse the statements.
    }

    if (cur.type == TokenType::END)
      fatalError("Unexpected EOF in state body!");
    consume();
    expectConsumeEOLs();
  }

  return func;
}

PapyrusProperty* PapyrusParser::parseProperty(PapyrusScript* script, PapyrusObject* object, bool isConst, PapyrusType type) {
  auto prop = new PapyrusProperty();
  prop->isConst = isConst;
  prop->type = type;
  prop->name = expectConsumeIdent();

  bool isFullProp = true;
  bool hadDefaultValue = false;
  if (maybeConsume(TokenType::Equal)) {
    isFullProp = false;
    hadDefaultValue = true;
    prop->defaultValue = expectConsumePapyrusValue();
  } else if (isConst) {
    fatalError("A const property must have a value!");
  }

  if (cur.type == TokenType::kAuto) {
    isFullProp = false;
    consume();
    if (isConst)
      fatalError("A const property cannot be marked Auto!");
  } else if (cur.type == TokenType::kAutoReadOnly) {
    isFullProp = false;
    prop->isAutoReadOnly = true;
    consume();
    if (!hadDefaultValue)
      fatalError("An AutoReadOnly property must have a value!");
    if (isConst)
      fatalError("A const property cannot be marked AutoReadOnly!");
  }
  prop->userFlags = parseUserFlags(PapyrusUserFlags::Conditional | PapyrusUserFlags::Hidden | PapyrusUserFlags::Mandatory);
  expectConsumeEOLs();
  prop->documentationComment = maybeConsumeDocString();

  if (isFullProp) {
    if (isConst)
      fatalError("A full property can't be declared const!");

    for (int i = 0; i < 2; i++) {
      switch (cur.type) {
        case TokenType::kFunction:
          if (prop->writeFunction)
            fatalError("The set function for this property has already been defined!");
          consume();
          prop->writeFunction = parseFunction(script, object, nullptr, PapyrusType::None(), TokenType::kEndFunction);
          if (_stricmp(prop->writeFunction->name.c_str(), "set"))
            fatalError("The set function must be named \"Set\"!");
          if (prop->writeFunction->parameters.size() != 1)
            fatalError("The set function must have a single parameter!");
          if (prop->writeFunction->parameters[0]->type != prop->type)
            fatalError("The set function's parameter must be the same type as the property!");
          break;

        case TokenType::kBool:
        case TokenType::kFloat:
        case TokenType::kInt:
        case TokenType::kString:
        case TokenType::kVar:
        case TokenType::Identifier:
        {
          if (prop->readFunction)
            fatalError("The get function for this property has already been defined!");
          auto tp = expectConsumePapyrusType();
          if (tp != prop->type)
            fatalError("The return type of the get function must be the same as the property!");
          expectConsume(TokenType::kFunction);
          prop->readFunction = parseFunction(script, object, nullptr, tp, TokenType::kEndFunction);
          if (_stricmp(prop->readFunction->name.c_str(), "get"))
            fatalError("The get function must be named \"Get\"!");
          if (prop->readFunction->parameters.size() != 0)
            fatalError("The get function cannot have parameters!");
          break;
        }

        case TokenType::kEndProperty:
          break;

        default:
          fatalError("Expected the get/set functions of a full property!");
      }
    }

    expectConsume(TokenType::kEndProperty);
    expectConsumeEOLs();
  }

  return prop;
}

PapyrusVariable* PapyrusParser::parseVariable(PapyrusScript* script, PapyrusObject* object, bool isConst, PapyrusType type) {
  auto var = new PapyrusVariable();
  var->isConst = isConst;
  var->type = type;
  var->name = expectConsumeIdent();

  if (maybeConsume(TokenType::Equal)) {
    var->defaultValue = expectConsumePapyrusValue();
  } else if (isConst) {
    fatalError("A constant variable must have a value!");
  }

  var->userFlags = parseUserFlags(PapyrusUserFlags::Conditional);
  expectConsumeEOLs();
  return var;
}


PapyrusType PapyrusParser::expectConsumePapyrusType() {
  switch (cur.type) {
    case TokenType::kBool:
      consume();
      return PapyrusType::Bool();
    case TokenType::kFloat:
      consume();
      return PapyrusType::Float();
    case TokenType::kInt:
      consume();
      return PapyrusType::Int();
    case TokenType::kString:
      consume();
      return PapyrusType::String();
    case TokenType::kVar:
      consume();
      return PapyrusType::Var();
    case TokenType::Identifier:
    {
      auto ident = expectConsumeIdent();
      if (cur.type == TokenType::LSquare) {
        consume();
        expectConsume(TokenType::RSqaure);
        return PapyrusType::Array(ident);
      }
      return PapyrusType::Unresolved(ident);
    }

    default:
      fatalError("Expected a type!");
  }
}

PapyrusValue PapyrusParser::expectConsumePapyrusValue() {
  PapyrusValue val;

  switch (cur.type) {
    case TokenType::Float:
      val.type = PapyrusValueType::Float;
      val.f = cur.fValue;
      consume();
      return val;
    case TokenType::Integer:
      val.type = PapyrusValueType::Integer;
      val.i = cur.iValue;
      consume();
      return val;
    case TokenType::String:
      val.type = PapyrusValueType::String;
      val.s = cur.sValue;
      consume();
      return val;
    case TokenType::kNone:
      val.type = PapyrusValueType::None;
      consume();
      return val;
    case TokenType::kTrue:
      val.type = PapyrusValueType::Bool;
      val.b = true;
      consume();
      return val;
    case TokenType::kFalse:
      val.type = PapyrusValueType::Bool;
      val.b = false;
      consume();
      return val;

    default:
      fatalError("Expected a default value!");
  }
}

static std::map<std::string, PapyrusUserFlags, CaselessStringComparer> userFlagMap {
  { "hidden", PapyrusUserFlags::Hidden },
  { "conditional", PapyrusUserFlags::Conditional },
  { "collapsedonref", PapyrusUserFlags::CollapsedOnRef },
  { "collapsedonbase", PapyrusUserFlags::CollapsedOnBase },
  { "mandatory", PapyrusUserFlags::Mandatory },
};

PapyrusUserFlags PapyrusParser::parseUserFlags(PapyrusUserFlags validFlags) {
  auto flags = PapyrusUserFlags::None;
  while (cur.type == TokenType::Identifier) {
    auto a = userFlagMap.find(cur.sValue);
    if (a == userFlagMap.end())
      fatalError("Unknown flag '" + cur.sValue + "'!");

    if ((validFlags & a->second) != a->second)
      fatalError("The flag '" + cur.sValue + "' is not valid in this location.");
    flags |= a->second;
    consume();
  }
  return flags;
}

}}}
