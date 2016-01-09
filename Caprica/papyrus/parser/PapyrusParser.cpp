#include <papyrus/parser/PapyrusParser.h>

#include <vector>

#include <boost/filesystem.hpp>

#include <common/CaselessStringComparer.h>

#include <papyrus/PapyrusObject.h>

#include <papyrus/expressions/PapyrusArrayIndexExpression.h>
#include <papyrus/expressions/PapyrusArrayLengthExpression.h>
#include <papyrus/expressions/PapyrusBinaryOpExpression.h>
#include <papyrus/expressions/PapyrusCastExpression.h>
#include <papyrus/expressions/PapyrusFunctionCallExpression.h>
#include <papyrus/expressions/PapyrusIdentifierExpression.h>
#include <papyrus/expressions/PapyrusIsExpression.h>
#include <papyrus/expressions/PapyrusLiteralExpression.h>
#include <papyrus/expressions/PapyrusMemberAccessExpression.h>
#include <papyrus/expressions/PapyrusNewArrayExpression.h>
#include <papyrus/expressions/PapyrusNewStructExpression.h>
#include <papyrus/expressions/PapyrusParentExpression.h>
#include <papyrus/expressions/PapyrusSelfExpression.h>
#include <papyrus/expressions/PapyrusUnaryOpExpression.h>

#include <papyrus/statements/PapyrusAssignStatement.h>
#include <papyrus/statements/PapyrusDeclareStatement.h>
#include <papyrus/statements/PapyrusExpressionStatement.h>
#include <papyrus/statements/PapyrusIfStatement.h>
#include <papyrus/statements/PapyrusReturnStatement.h>
#include <papyrus/statements/PapyrusWhileStatement.h>


namespace caprica { namespace papyrus { namespace parser {

PapyrusScript* PapyrusParser::parseScript() {
  auto script = new PapyrusScript();
  script->sourceFileName = boost::filesystem::canonical(boost::filesystem::absolute(filename)).make_preferred().string();
  script->objects.push_back(parseObject(script));
  return script;
}

PapyrusObject* PapyrusParser::parseObject(PapyrusScript* script) {
  auto loc = cur.location;
  bool isConst = false;
  maybeConsumeEOLs();
  if (maybeConsume(TokenType::kConst))
    isConst = true;
  expectConsume(TokenType::kScriptName);
  auto name = expectConsumeIdent();
  if (_stricmp(boost::filesystem::basename(script->sourceFileName).c_str(), name.c_str()))
    CapricaError::error(cur.location, "The script name '%s' must match the name of the file '%s'!", name.c_str(), boost::filesystem::basename(script->sourceFileName).c_str());

  PapyrusObject* obj;
  if (maybeConsume(TokenType::kExtends)) {
    auto eLoc = cur.location;
    obj = new PapyrusObject(loc, PapyrusType::Unresolved(eLoc, expectConsumeIdent()));
  } else {
    obj = new PapyrusObject(loc, PapyrusType::None(cur.location));

    // Otherwise we get to have some fun and generate GotoState and GetState.
    auto getState = new PapyrusFunction(cur.location, PapyrusType::String(cur.location));
    getState->functionType = PapyrusFunctionType::Function;
    getState->parentObject = obj;
    getState->name = "GetState";
    obj->getRootState()->functions.push_back(getState);

    auto gotoState = new PapyrusFunction(cur.location, PapyrusType::None(cur.location));
    gotoState->functionType = PapyrusFunctionType::Function;
    gotoState->parentObject = obj;
    gotoState->name = "GotoState";
    auto param = new PapyrusFunctionParameter(cur.location, PapyrusType::String(cur.location));
    param->name = "asNewState";
    gotoState->parameters.push_back(param);
    obj->getRootState()->functions.push_back(gotoState);
  }
  obj->isConst = isConst;
  obj->name = name;
  obj->userFlags = maybeConsumeUserFlags(PapyrusUserFlags::Conditional | PapyrusUserFlags::Default | PapyrusUserFlags::Hidden);
  expectConsumeEOLs();
  obj->documentationString = maybeConsumeDocString();

  while (cur.type != TokenType::END) {
    bool isConst = false;
    switch (cur.type) {
      case TokenType::kImport:
      {
        consume();
        auto eLoc = cur.location;
        obj->imports.push_back(std::make_pair(eLoc, expectConsumeIdent()));
        expectConsumeEOLs();
        break;
      }

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
        obj->getRootState()->functions.push_back(parseFunction(script, obj, obj->getRootState(), PapyrusType::None(cur.location), TokenType::kEndEvent));
        break;
      case TokenType::kFunction:
        consume();
        obj->getRootState()->functions.push_back(parseFunction(script, obj, obj->getRootState(), PapyrusType::None(cur.location), TokenType::kEndFunction));
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
            CapricaError::error(cur.location, "The return type of a function cannot be marked const!");
          consume();
          obj->getRootState()->functions.push_back(parseFunction(script, obj, obj->getRootState(), tp, TokenType::kEndFunction));
        } else if (cur.type == TokenType::kProperty) {
          if (isConst)
            CapricaError::error(cur.location, "A property cannot be marked const!");
          consume();
          obj->getRootPropertyGroup()->properties.push_back(parseProperty(script, obj, tp));
        } else {
          obj->variables.push_back(parseVariable(script, obj, isConst, tp));
        }
        break;
      }

      default:
        CapricaError::fatal(cur.location, "Unexpected token '%s'!", cur.prettyString().c_str());
    }
  }

  return obj;
}

PapyrusState* PapyrusParser::parseState(PapyrusScript* script, PapyrusObject* object, bool isAuto) {
  auto state = new PapyrusState(cur.location);
  state->name = expectConsumeIdent();
  if (isAuto) {
    if (object->autoState != nullptr)
      CapricaError::error(cur.location, "Only one state can be declared auto. '%s' was already declared as the auto state.", object->autoState->name.c_str());
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
        state->functions.push_back(parseFunction(script, object, state, PapyrusType::None(cur.location), TokenType::kEndEvent));
        break;
      case TokenType::kFunction:
        consume();
        state->functions.push_back(parseFunction(script, object, state, PapyrusType::None(cur.location), TokenType::kEndFunction));
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
        CapricaError::fatal(cur.location, "Expected an event or function, got '%s'!", cur.prettyString().c_str());
    }
  }

Return:
  return state;
}

PapyrusStruct* PapyrusParser::parseStruct(PapyrusScript* script, PapyrusObject* object) {
  auto struc = new PapyrusStruct(cur.location);
  struc->parentObject = object;
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
        auto m = parseStructMember(script, object, struc, isConst, tp);
        for (auto sm : struc->members) {
          if (!_stricmp(sm->name.c_str(), m->name.c_str()))
            CapricaError::error(m->location, "A member named '%s' was already defined in '%s'.", m->name.c_str(), struc->name.c_str());
        }
        struc->members.push_back(m);
        break;
      }

      default:
        CapricaError::fatal(cur.location, "Unexpected token '%s' while parsing struct!", cur.prettyString().c_str());
    }
  }

Return:
  return struc;
}

PapyrusStructMember* PapyrusParser::parseStructMember(PapyrusScript* script, PapyrusObject* object, PapyrusStruct* struc, bool isConst, PapyrusType tp) {
  auto mem = new PapyrusStructMember(cur.location, tp);
  mem->isConst = isConst;
  mem->name = expectConsumeIdent();

  if (maybeConsume(TokenType::Equal)) {
    mem->defaultValue = expectConsumePapyrusValue();
  } else if (isConst) {
    CapricaError::error(cur.location, "A constant member must have a value!");
  }

  mem->userFlags = maybeConsumeUserFlags(PapyrusUserFlags::Conditional | PapyrusUserFlags::Hidden);
  expectConsumeEOLs();
  mem->documentationString = maybeConsumeDocString();
  return mem;
}

PapyrusPropertyGroup* PapyrusParser::parsePropertyGroup(PapyrusScript* script, PapyrusObject* object) {
  auto group = new PapyrusPropertyGroup(cur.location);
  group->name = expectConsumeIdent();
  group->userFlags = maybeConsumeUserFlags(PapyrusUserFlags::CollapsedOnBase | PapyrusUserFlags::CollapsedOnRef | PapyrusUserFlags::Hidden);
  expectConsumeEOLs();
  group->documentationComment = maybeConsumeDocString();

  while (true) {
    bool isConst = false;
    switch (cur.type) {
      case TokenType::kEndPropertyGroup:
        consume();
        expectConsumeEOLs();
        goto Return;

      case TokenType::kBool:
      case TokenType::kFloat:
      case TokenType::kInt:
      case TokenType::kString:
      case TokenType::kVar:
      case TokenType::Identifier:
      {
        auto tp = expectConsumePapyrusType();
        expectConsume(TokenType::kProperty);
        group->properties.push_back(parseProperty(script, object, tp));
        break;
      }

      default:
        CapricaError::fatal(cur.location, "Unexpected token '%s' while parsing property group!", cur.prettyString().c_str());
    }
  }

Return:
  return group;
}

PapyrusProperty* PapyrusParser::parseProperty(PapyrusScript* script, PapyrusObject* object, PapyrusType type) {
  auto prop = new PapyrusProperty(cur.location, type);
  prop->name = expectConsumeIdent();

  bool isFullProp = true;
  bool hadDefaultValue = false;
  if (maybeConsume(TokenType::Equal)) {
    isFullProp = false;
    hadDefaultValue = true;
    prop->defaultValue = expectConsumePapyrusValue();
  }

  if (cur.type == TokenType::kAuto) {
    isFullProp = false;
    prop->isAuto = true;
    consume();
  } else if (cur.type == TokenType::kAutoReadOnly) {
    isFullProp = false;
    prop->isAuto = true;
    prop->isReadOnly = true;
    consume();
    if (!hadDefaultValue)
      CapricaError::error(cur.location, "An AutoReadOnly property must have a value!");
  }
  prop->userFlags = maybeConsumeUserFlags(PapyrusUserFlags::Conditional | PapyrusUserFlags::Hidden | PapyrusUserFlags::Mandatory);
  expectConsumeEOLs();
  prop->documentationComment = maybeConsumeDocString();

  if (isFullProp) {
    for (int i = 0; i < 2; i++) {
      switch (cur.type) {
        case TokenType::kFunction:
          if (prop->writeFunction)
            CapricaError::error(cur.location, "The set function for this property has already been defined!");
          consume();
          prop->writeFunction = parseFunction(script, object, nullptr, PapyrusType::None(cur.location), TokenType::kEndFunction);
          prop->writeFunction->functionType = PapyrusFunctionType::Setter;
          if (_stricmp(prop->writeFunction->name.c_str(), "set"))
            CapricaError::error(cur.location, "The set function must be named \"Set\"!");
          if (prop->writeFunction->parameters.size() != 1)
            CapricaError::error(cur.location, "The set function must have a single parameter!");
          if (prop->writeFunction->parameters[0]->type != prop->type)
            CapricaError::error(cur.location, "The set function's parameter must be the same type as the property!");
          break;

        case TokenType::kBool:
        case TokenType::kFloat:
        case TokenType::kInt:
        case TokenType::kString:
        case TokenType::kVar:
        case TokenType::Identifier:
        {
          if (prop->readFunction)
            CapricaError::error(cur.location, "The get function for this property has already been defined!");
          auto tp = expectConsumePapyrusType();
          if (tp != prop->type)
            CapricaError::error(cur.location, "The return type of the get function must be the same as the property!");
          expectConsume(TokenType::kFunction);
          prop->readFunction = parseFunction(script, object, nullptr, tp, TokenType::kEndFunction);
          prop->readFunction->functionType = PapyrusFunctionType::Getter;
          if (_stricmp(prop->readFunction->name.c_str(), "get"))
            CapricaError::error(cur.location, "The get function must be named \"Get\"!");
          if (prop->readFunction->parameters.size() != 0)
            CapricaError::error(cur.location, "The get function cannot have parameters!");
          break;
        }

        case TokenType::kEndProperty:
          break;

        default:
          CapricaError::fatal(cur.location, "Expected the get/set functions of a full property, got '%s'!", cur.prettyString().c_str());
      }
    }

    expectConsume(TokenType::kEndProperty);
    expectConsumeEOLs();
  }

  return prop;
}

PapyrusVariable* PapyrusParser::parseVariable(PapyrusScript* script, PapyrusObject* object, bool isConst, PapyrusType type) {
  auto var = new PapyrusVariable(cur.location, type);
  var->isConst = isConst;
  var->name = expectConsumeIdent();

  if (maybeConsume(TokenType::Equal)) {
    var->defaultValue = expectConsumePapyrusValue();
  } else if (isConst) {
    CapricaError::error(cur.location, "A constant variable must have a value!");
  }

  var->userFlags = maybeConsumeUserFlags(PapyrusUserFlags::Conditional);
  expectConsumeEOLs();
  return var;
}

PapyrusFunction* PapyrusParser::parseFunction(PapyrusScript* script, PapyrusObject* object, PapyrusState* state, PapyrusType returnType, TokenType endToken) {
  auto func = new PapyrusFunction(cur.location, returnType);
  if (endToken == TokenType::kEndFunction)
    func->functionType = PapyrusFunctionType::Function;
  else if (endToken == TokenType::kEndEvent)
    func->functionType = PapyrusFunctionType::Event;
  else
    CapricaError::logicalFatal("Unknown end token for a parseFunction call!");
  func->parentObject = object;
  func->name = expectConsumeIdent();
  expectConsume(TokenType::LParen);

  if (cur.type != TokenType::RParen) {
    do {
      maybeConsume(TokenType::Comma);

      auto param = new PapyrusFunctionParameter(cur.location, expectConsumePapyrusType());
      param->name = expectConsumeIdent();
      if (maybeConsume(TokenType::Equal)) {
        param->hasDefaultValue = true;
        param->defaultValue = expectConsumePapyrusValue();
      }
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
      CapricaError::error(cur.location, "This function was already declared global!");
    func->isGlobal = true;
  }

  func->userFlags = maybeConsumeUserFlags(PapyrusUserFlags::None);
  expectConsumeEOLs();
  func->documentationComment = maybeConsumeDocString();
  if (!func->isNative) {
    while (cur.type != endToken && cur.type != TokenType::END) {
      func->statements.push_back(parseStatement(func));
    }

    if (cur.type == TokenType::END)
      CapricaError::fatal(cur.location, "Unexpected EOF in state body!");
    consume();
    expectConsumeEOLs();
  }

  return func;
}

statements::PapyrusStatement* PapyrusParser::parseStatement(PapyrusFunction* func) {
  switch (cur.type) {
    case TokenType::kReturn:
    {
      auto ret = new statements::PapyrusReturnStatement(cur.location);
      consume();
      if (cur.type != TokenType::EOL)
        ret->returnValue = parseExpression(func);
      expectConsumeEOLs();
      return ret;
    }

    case TokenType::kIf:
    {
      auto ret = new statements::PapyrusIfStatement(cur.location);
      consume();
      while (true) {
        auto cond = parseExpression(func);
        expectConsumeEOLs();
        std::vector<statements::PapyrusStatement*> curStatements{ };
        while (cur.type != TokenType::kElseIf && cur.type != TokenType::kElse && cur.type != TokenType::kEndIf) {
          curStatements.push_back(parseStatement(func));
        }
        ret->ifBodies.push_back(std::make_pair(cond, curStatements));
        if (cur.type == TokenType::kElseIf) {
          consume();
          continue;
        }
        if (cur.type == TokenType::kElse) {
          consume();
          expectConsumeEOLs();
          while (cur.type != TokenType::kEndIf) {
            ret->elseStatements.push_back(parseStatement(func));
          }
        }
        expectConsume(TokenType::kEndIf);
        expectConsumeEOLs();
        goto Return;
      }
    Return:
      return ret;
    }

    case TokenType::kWhile:
    {
      auto ret = new statements::PapyrusWhileStatement(cur.location);
      consume();
      ret->condition = parseExpression(func);
      expectConsumeEOLs();
      while (cur.type != TokenType::kEndWhile)
        ret->body.push_back(parseStatement(func));
      expectConsume(TokenType::kEndWhile);
      expectConsumeEOLs();
      return ret;
    }

    case TokenType::kBool:
    case TokenType::kFloat:
    case TokenType::kInt:
    case TokenType::kString:
    case TokenType::kVar:
    {
      auto eLoc = cur.location;
      auto ret = new statements::PapyrusDeclareStatement(eLoc, expectConsumePapyrusType());
      ret->name = expectConsumeIdent();
      if (maybeConsume(TokenType::Equal))
        ret->initialValue = parseExpression(func);
      expectConsumeEOLs();
      return ret;
    }

    case TokenType::Identifier:
    default:
    {
      if (cur.type == TokenType::Identifier && (peekToken().type == TokenType::Identifier ||  (peekToken().type == TokenType::LSquare && peekToken(1).type == TokenType::RSquare && peekToken(2).type == TokenType::Identifier))) {
        auto eLoc = cur.location;
        auto ret = new statements::PapyrusDeclareStatement(eLoc, expectConsumePapyrusType());
        ret->name = expectConsumeIdent();
        if (maybeConsume(TokenType::Equal))
          ret->initialValue = parseExpression(func);
        expectConsumeEOLs();
        return ret;
      }

      auto expr = parseExpression(func);
      auto op = statements::PapyrusAssignOperatorType::None;
      switch (cur.type) {
        case TokenType::Equal:
          op = statements::PapyrusAssignOperatorType::Assign;
          goto AssignStatementCommon;
        case TokenType::PlusEqual:
          op = statements::PapyrusAssignOperatorType::Add;
          goto AssignStatementCommon;
        case TokenType::MinusEqual:
          op = statements::PapyrusAssignOperatorType::Subtract;
          goto AssignStatementCommon;
        case TokenType::MulEqual:
          op = statements::PapyrusAssignOperatorType::Multiply;
          goto AssignStatementCommon;
        case TokenType::DivEqual:
          op = statements::PapyrusAssignOperatorType::Divide;
          goto AssignStatementCommon;
        case TokenType::ModEqual:
          op = statements::PapyrusAssignOperatorType::Modulus;
          goto AssignStatementCommon;
        AssignStatementCommon:
        {
          auto assStat = new statements::PapyrusAssignStatement(cur.location);
          assStat->lValue = expr;
          consume();
          assStat->operation = op;
          assStat->rValue = parseExpression(func);
          expectConsumeEOLs();
          return assStat;
        }

        default:
        {
          auto exprStat = new statements::PapyrusExpressionStatement(expr->location);
          exprStat->expression = expr;
          expectConsumeEOLs();
          return exprStat;
        }
      }
    }
  }
}

expressions::PapyrusExpression* PapyrusParser::parseExpression(PapyrusFunction* func) {
  auto expr = parseAndExpression(func);
  while (cur.type == TokenType::BooleanOr) {
    auto binExpr = new expressions::PapyrusBinaryOpExpression(cur.location);
    binExpr->left = expr;
    binExpr->operation = expressions::PapyrusBinaryOperatorType::BooleanOr;
    consume();
    binExpr->right = parseAndExpression(func);
    expr = binExpr;
  }
  return expr;
}

expressions::PapyrusExpression* PapyrusParser::parseAndExpression(PapyrusFunction* func) {
  auto expr = parseCmpExpression(func);
  while (cur.type == TokenType::BooleanAnd) {
    auto binExpr = new expressions::PapyrusBinaryOpExpression(cur.location);
    binExpr->left = expr;
    binExpr->operation = expressions::PapyrusBinaryOperatorType::BooleanAnd;
    consume();
    binExpr->right = parseCmpExpression(func);
    expr = binExpr;
  }
  return expr;
}

expressions::PapyrusExpression* PapyrusParser::parseCmpExpression(PapyrusFunction* func) {
  auto expr = parseAddExpression(func);
  while (true) {
    expressions::PapyrusBinaryOperatorType op = expressions::PapyrusBinaryOperatorType::None;
    switch (cur.type) {
      case TokenType::CmpEq:
        op = expressions::PapyrusBinaryOperatorType::CmpEq;
        goto OperatorCommon;
      case TokenType::CmpNeq:
        op = expressions::PapyrusBinaryOperatorType::CmpNeq;
        goto OperatorCommon;
      case TokenType::CmpLt:
        op = expressions::PapyrusBinaryOperatorType::CmpLt;
        goto OperatorCommon;
      case TokenType::CmpLte:
        op = expressions::PapyrusBinaryOperatorType::CmpLte;
        goto OperatorCommon;
      case TokenType::CmpGt:
        op = expressions::PapyrusBinaryOperatorType::CmpGt;
        goto OperatorCommon;
      case TokenType::CmpGte:
        op = expressions::PapyrusBinaryOperatorType::CmpGte;
        goto OperatorCommon;

      OperatorCommon:
      {
        auto binExpr = new expressions::PapyrusBinaryOpExpression(cur.location);
        binExpr->left = expr;
        binExpr->operation = op;
        consume();
        binExpr->right = parseAddExpression(func);
        expr = binExpr;
        break;
      }

      default:
        goto Return;
    }
  }
Return:
  return expr;
}

expressions::PapyrusExpression* PapyrusParser::parseAddExpression(PapyrusFunction* func) {
  auto expr = parseMultExpression(func);
  while (true) {
    auto op = expressions::PapyrusBinaryOperatorType::None;
    switch (cur.type) {
      case TokenType::Plus:
        op = expressions::PapyrusBinaryOperatorType::Add;
        goto OperatorCommon;
      case TokenType::Minus:
        op = expressions::PapyrusBinaryOperatorType::Subtract;
        goto OperatorCommon;

      OperatorCommon:
      {
        auto binExpr = new expressions::PapyrusBinaryOpExpression(cur.location);
        binExpr->left = expr;
        binExpr->operation = op;
        consume();
        binExpr->right = parseMultExpression(func);
        expr = binExpr;
        break;
      }

      default:
        goto Return;
    }
  }
Return:
  return expr;
}

expressions::PapyrusExpression* PapyrusParser::parseMultExpression(PapyrusFunction* func) {
  auto expr = parseUnaryExpression(func);
  while (true) {
    auto op = expressions::PapyrusBinaryOperatorType::None;
    switch (cur.type) {
      case TokenType::Mul:
        op = expressions::PapyrusBinaryOperatorType::Multiply;
        goto OperatorCommon;
      case TokenType::Div:
        op = expressions::PapyrusBinaryOperatorType::Divide;
        goto OperatorCommon;
      case TokenType::Mod:
        op = expressions::PapyrusBinaryOperatorType::Modulus;
        goto OperatorCommon;

      OperatorCommon:
      {
        auto binExpr = new expressions::PapyrusBinaryOpExpression(cur.location);
        binExpr->left = expr;
        binExpr->operation = op;
        consume();
        binExpr->right = parseUnaryExpression(func);
        expr = binExpr;
        break;
      }

      default:
        goto Return;
    }
  }
Return:
  return expr;
}

expressions::PapyrusExpression* PapyrusParser::parseUnaryExpression(PapyrusFunction* func) {
  auto op = expressions::PapyrusUnaryOperatorType::None;
  switch (cur.type) {
    case TokenType::Exclaim:
      op = expressions::PapyrusUnaryOperatorType::Not;
      goto OperatorCommon;
    case TokenType::Minus:
      op = expressions::PapyrusUnaryOperatorType::Negate;
      goto OperatorCommon;

    OperatorCommon:
    {
      auto unExpr = new expressions::PapyrusUnaryOpExpression(cur.location);
      unExpr->operation = op;
      consume();
      unExpr->innerExpression = parseCastExpression(func);
      return unExpr;
    }
    default:
      return parseCastExpression(func);
  }
}

expressions::PapyrusExpression* PapyrusParser::parseCastExpression(PapyrusFunction* func) {
  auto expr = parseDotExpression(func);

  if (cur.type == TokenType::kIs) {
    auto loc = cur.location;
    consume();
    auto isExpr = new expressions::PapyrusIsExpression(loc, expectConsumePapyrusType());
    isExpr->innerExpression = expr;
    expr = isExpr;
  } else if (cur.type == TokenType::kAs) {
    auto loc = cur.location;
    consume();
    auto castExpr = new expressions::PapyrusCastExpression(loc, expectConsumePapyrusType());
    castExpr->innerExpression = expr;
    expr = castExpr;
  }

  return expr;
}

expressions::PapyrusExpression* PapyrusParser::parseDotExpression(PapyrusFunction* func) {
  switch (cur.type) {
    case TokenType::Float:
    case TokenType::Integer:
    case TokenType::String:
    case TokenType::kNone:
    case TokenType::kTrue:
    case TokenType::kFalse:
    {
      auto eLoc = cur.location;
      return new expressions::PapyrusLiteralExpression(eLoc, expectConsumePapyrusValue());
    }

    default:
    {
      auto expr = parseArrayExpression(func);
      while (cur.type == TokenType::Dot) {
        auto maExpr = new expressions::PapyrusMemberAccessExpression(cur.location);
        consume();
        maExpr->baseExpression = expr;
        maExpr->accessExpression = parseFuncOrIdExpression(func);

        if (cur.type == TokenType::LSquare) {
          auto aiExpr = new expressions::PapyrusArrayIndexExpression(cur.location);
          consume();
          aiExpr->baseExpression = maExpr;
          aiExpr->indexExpression = parseExpression(func);
          expectConsume(TokenType::RSquare);
          expr = aiExpr;
        }
        else {
          expr = maExpr;
        }
      }
      return expr;
    }
  }
}

expressions::PapyrusExpression* PapyrusParser::parseArrayExpression(PapyrusFunction* func) {
  auto expr = parseAtomExpression(func);
  if (cur.type == TokenType::LSquare) {
    auto aiExpr = new expressions::PapyrusArrayIndexExpression(cur.location);
    consume();
    aiExpr->baseExpression = expr;
    aiExpr->indexExpression = parseExpression(func);
    expectConsume(TokenType::RSquare);
    return aiExpr;
  }
  return expr;
}

expressions::PapyrusExpression* PapyrusParser::parseAtomExpression(PapyrusFunction* func) {
  switch (cur.type) {
    case TokenType::LParen:
    {
      consume();
      auto expr = parseExpression(func);
      expectConsume(TokenType::RParen);
      return expr;
    }
    
    case TokenType::kNew:
    {
      auto loc = cur.location;
      consume();
      auto tp = expectConsumePapyrusType();
      if (maybeConsume(TokenType::LSquare)) {
        auto nArrExpr = new expressions::PapyrusNewArrayExpression(loc, tp);
        nArrExpr->lengthExpression = parseExpression(func);
        expectConsume(TokenType::RSquare);
        return nArrExpr;
      }

      expectConsume(TokenType::LParen);
      expectConsume(TokenType::RParen);
      return new expressions::PapyrusNewStructExpression(loc, tp);
    }

    default:
      return parseFuncOrIdExpression(func);
  }
}

expressions::PapyrusExpression* PapyrusParser::parseFuncOrIdExpression(PapyrusFunction* func) {
  switch (cur.type) {
    case TokenType::kLength:
    {
      auto lenExpr = new expressions::PapyrusArrayLengthExpression(cur.location);
      consume();
      return lenExpr;
    }
    case TokenType::kParent:
    {
      auto parExpr = new expressions::PapyrusParentExpression(cur.location, func->parentObject->parentClass);
      consume();
      return parExpr;
    }
    case TokenType::kSelf:
    {
      auto selfExpr = new expressions::PapyrusSelfExpression(cur.location, PapyrusType::ResolvedObject(cur.location, func->parentObject));
      consume();
      return selfExpr;
    }
    case TokenType::Identifier:
    {
      if (peekToken().type == TokenType::LParen) {
        auto eLoc = cur.location;
        auto fCallExpr = new expressions::PapyrusFunctionCallExpression(eLoc, PapyrusIdentifier::Unresolved(eLoc, expectConsumeIdent()));
        expectConsume(TokenType::LParen);

        if (cur.type != TokenType::RParen) {
          do {
            maybeConsume(TokenType::Comma);

            auto param = new expressions::PapyrusFunctionCallExpression::Parameter();
            if (cur.type == TokenType::Identifier && peekToken().type == TokenType::Equal) {
              param->name = expectConsumeIdent();
              expectConsume(TokenType::Equal);
            }
            param->value = parseExpression(func);
            fCallExpr->arguments.push_back(param);
          } while (cur.type == TokenType::Comma);
        }
        expectConsume(TokenType::RParen);

        return fCallExpr;
      } else {
        auto eLoc = cur.location;
        return new expressions::PapyrusIdentifierExpression(eLoc, PapyrusIdentifier::Unresolved(eLoc, expectConsumeIdent()));
      }
    }
    default:
      CapricaError::fatal(cur.location, "Unexpected token '%s'!", cur.prettyString().c_str());
  }
}

PapyrusType PapyrusParser::expectConsumePapyrusType() {
  PapyrusType tp = PapyrusType::Default();
  switch (cur.type) {
    case TokenType::kBool:
      tp = PapyrusType::Bool(cur.location);
      consume();
      break;
    case TokenType::kFloat:
      tp = PapyrusType::Float(cur.location);
      consume();
      break;
    case TokenType::kInt:
      tp = PapyrusType::Int(cur.location);
      consume();
      break;
    case TokenType::kString:
      tp = PapyrusType::String(cur.location);
      consume();
      break;
    case TokenType::kVar:
      tp = PapyrusType::Var(cur.location);
      consume();
      break;
    case TokenType::Identifier:
    {
      auto eLoc = cur.location;
      tp = PapyrusType::Unresolved(eLoc, expectConsumeIdent());
      break;
    }

    default:
      CapricaError::fatal(cur.location, "Expected a type, got '%s'!", cur.prettyString().c_str());
  }

  if (cur.type == TokenType::LSquare && peekToken().type == TokenType::RSquare) {
    consume();
    expectConsume(TokenType::RSquare);
    return PapyrusType::Array(tp.location, std::make_shared<PapyrusType>(tp));
  }
  return tp;
}

PapyrusValue PapyrusParser::expectConsumePapyrusValue() {
  PapyrusValue val{ cur.location };

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
      CapricaError::fatal(cur.location, "Expected a default value, got '%s'!", cur.prettyString().c_str());
  }
}

static std::map<std::string, PapyrusUserFlags, CaselessStringComparer> userFlagMap {
  { "hidden", PapyrusUserFlags::Hidden },
  { "conditional", PapyrusUserFlags::Conditional },
  { "default", PapyrusUserFlags::Default },
  { "collapsedonref", PapyrusUserFlags::CollapsedOnRef },
  { "collapsedonbase", PapyrusUserFlags::CollapsedOnBase },
  { "mandatory", PapyrusUserFlags::Mandatory },
};

PapyrusUserFlags PapyrusParser::maybeConsumeUserFlags(PapyrusUserFlags validFlags) {
  auto flags = PapyrusUserFlags::None;
  while (cur.type == TokenType::Identifier) {
    auto a = userFlagMap.find(cur.sValue);
    if (a == userFlagMap.end())
      CapricaError::fatal(cur.location, "Unknown flag '%s'!", cur.sValue.c_str());

    if ((validFlags & a->second) != a->second)
      CapricaError::error(cur.location, "The flag '%s' is not valid in this location.", cur.sValue.c_str());
    flags |= a->second;
    consume();
  }
  return flags;
}

}}}
