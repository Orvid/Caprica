#include <papyrus/parser/PapyrusParser.h>

#include <vector>

#include <boost/filesystem.hpp>

#include <papyrus/PapyrusObject.h>

#include <papyrus/expressions/PapyrusArrayIndexExpression.h>
#include <papyrus/expressions/PapyrusBinaryOpExpression.h>
#include <papyrus/expressions/PapyrusCastExpression.h>
#include <papyrus/expressions/PapyrusIdentifierExpression.h>
#include <papyrus/expressions/PapyrusLiteralExpression.h>
//#include <papyrus/expressions/PapyrusMemberAccessExpression.h>
#include <papyrus/expressions/PapyrusNewArrayExpression.h>
#include <papyrus/expressions/PapyrusNewStructExpression.h>
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
  script->sourceFileName = boost::filesystem::absolute(filename).string();
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
  obj->name = expectConsumeIdent();
  if (_stricmp(boost::filesystem::basename(script->sourceFileName).c_str(), obj->name.c_str()))
    fatalError("The script name must match the name of the file!");
  if (maybeConsume(TokenType::kExtends))
    obj->parentClass = PapyrusType::Unresolved(expectConsumeIdent());
  obj->userFlags = maybeConsumeUserFlags(PapyrusUserFlags::Hidden | PapyrusUserFlags::Conditional);
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
          if (isConst)
            fatalError("A property cannot be marked const!");
          consume();
          obj->getRootPropertyGroup()->properties.push_back(parseProperty(script, obj, tp));
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

  mem->userFlags = maybeConsumeUserFlags(PapyrusUserFlags::Conditional);
  expectConsumeEOLs();
  mem->documentationString = maybeConsumeDocString();
  return mem;
}

PapyrusPropertyGroup* PapyrusParser::parsePropertyGroup(PapyrusScript* script, PapyrusObject* object) {
  auto group = new PapyrusPropertyGroup();
  group->name = expectConsumeIdent();
  group->userFlags = maybeConsumeUserFlags(PapyrusUserFlags::Hidden);
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
        fatalError("Unexpected token while parsing property group!");
    }
  }

Return:
  return group;
}

PapyrusProperty* PapyrusParser::parseProperty(PapyrusScript* script, PapyrusObject* object, PapyrusType type) {
  auto prop = new PapyrusProperty();
  prop->type = type;
  prop->location = cur.getLocation();
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
      fatalError("An AutoReadOnly property must have a value!");
  }
  prop->userFlags = maybeConsumeUserFlags(PapyrusUserFlags::Conditional | PapyrusUserFlags::Hidden | PapyrusUserFlags::Mandatory);
  expectConsumeEOLs();
  prop->documentationComment = maybeConsumeDocString();

  if (isFullProp) {
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

  var->userFlags = maybeConsumeUserFlags(PapyrusUserFlags::Conditional);
  expectConsumeEOLs();
  return var;
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

  func->userFlags = maybeConsumeUserFlags(PapyrusUserFlags::None);
  expectConsumeEOLs();
  func->documentationComment = maybeConsumeDocString();
  if (!func->isNative) {
    while (cur.type != endToken && cur.type != TokenType::END) {
      func->statements.push_back(parseStatement(func));
    }

    if (cur.type == TokenType::END)
      fatalError("Unexpected EOF in state body!");
    consume();
    expectConsumeEOLs();
  }

  return func;
}

statements::PapyrusStatement* PapyrusParser::parseStatement(PapyrusFunction* func) {
  switch (cur.type) {
    case TokenType::kReturn:
    {
      auto ret = new statements::PapyrusReturnStatement(cur.getLocation());
      consume();
      if (cur.type != TokenType::EOL)
        ret->returnValue = parseExpression(func);
      expectConsumeEOLs();
      return ret;
    }

    case TokenType::kIf:
    {
      auto ret = new statements::PapyrusIfStatement(cur.getLocation());
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
      auto ret = new statements::PapyrusWhileStatement(cur.getLocation());
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
      auto ret = new statements::PapyrusDeclareStatement(cur.getLocation());
      ret->type = expectConsumePapyrusType();
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
        auto ret = new statements::PapyrusDeclareStatement(cur.getLocation());
        ret->type = expectConsumePapyrusType();
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
          auto assStat = new statements::PapyrusAssignStatement(cur.getLocation());
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
    auto binExpr = new expressions::PapyrusBinaryOpExpression(cur.getLocation());
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
    auto binExpr = new expressions::PapyrusBinaryOpExpression(cur.getLocation());
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
        auto binExpr = new expressions::PapyrusBinaryOpExpression(cur.getLocation());
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
        auto binExpr = new expressions::PapyrusBinaryOpExpression(cur.getLocation());
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
        auto binExpr = new expressions::PapyrusBinaryOpExpression(cur.getLocation());
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
      auto unExpr = new expressions::PapyrusUnaryOpExpression(cur.getLocation());
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
  // Extension note: The Wiki claims that the official parser doesn't allow
  // immediate chaining of casts, but we allow it here because we can.
  while (cur.type == TokenType::kAs) {
    auto castExpr = new expressions::PapyrusCastExpression(cur.getLocation());
    consume();
    castExpr->innerExpression = expr;
    castExpr->targetType = expectConsumePapyrusType();
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
      auto lit = new expressions::PapyrusLiteralExpression(cur.getLocation());
      lit->value = expectConsumePapyrusValue();
      return lit;
    }

    default:
    {
      auto expr = parseArrayExpression(func);
      /*while (cur.type == TokenType::Dot) {
        auto maExpr = new expressions::PapyrusMemberAccessExpression(cur.getLocation());
        consume();
        maExpr->baseExpression = expr;
        maExpr->accessExpression = parseFuncOrIdExpression(func);
        expr = maExpr;
      }*/
      return expr;
    }
  }
}

expressions::PapyrusExpression* PapyrusParser::parseArrayExpression(PapyrusFunction* func) {
  auto expr = parseAtomExpression(func);
  if (cur.type == TokenType::LSquare) {
    auto aiExpr = new expressions::PapyrusArrayIndexExpression(cur.getLocation());
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
      auto loc = cur.getLocation();
      consume();
      auto tp = expectConsumePapyrusType();
      if (maybeConsume(TokenType::LSquare)) {
        auto nArrExpr = new expressions::PapyrusNewArrayExpression(loc);
        nArrExpr->type = tp;
        nArrExpr->lengthExpression = parseExpression(func);
        expectConsume(TokenType::RSquare);
        return nArrExpr;
      }

      expectConsume(TokenType::LParen);
      expectConsume(TokenType::RParen);
      auto nStructExpr = new expressions::PapyrusNewStructExpression(loc);
      nStructExpr->type = tp;
      return nStructExpr;
    }

    default:
      return parseFuncOrIdExpression(func);
  }
}

expressions::PapyrusExpression* PapyrusParser::parseArrayFuncOrIdExpression(PapyrusFunction* func) {
  auto expr = parseFuncOrIdExpression(func);
  if (cur.type == TokenType::LSquare) {
    auto aiExpr = new expressions::PapyrusArrayIndexExpression(cur.getLocation());
    consume();
    aiExpr->baseExpression = expr;
    aiExpr->indexExpression = parseExpression(func);
    expectConsume(TokenType::RSquare);
    return aiExpr;
  }
  return expr;
}

expressions::PapyrusExpression* PapyrusParser::parseFuncOrIdExpression(PapyrusFunction* func) {
  expect(TokenType::Identifier);
  auto idExpr = new expressions::PapyrusIdentifierExpression(cur.getLocation());
  PapyrusIdentifier id;
  id.type = PapyrusIdentifierType::Unresolved;
  id.name = cur.sValue;
  idExpr->identifier = id;
  consume();
  return idExpr;
}

PapyrusType PapyrusParser::expectConsumePapyrusType() {
  PapyrusType tp;
  switch (cur.type) {
    case TokenType::kBool:
      consume();
      tp = PapyrusType::Bool();
      break;
    case TokenType::kFloat:
      consume();
      tp = PapyrusType::Float();
      break;
    case TokenType::kInt:
      consume();
      tp = PapyrusType::Int();
      break;
    case TokenType::kString:
      consume();
      tp = PapyrusType::String();
      break;
    case TokenType::kVar:
      consume();
      tp = PapyrusType::Var();
      break;
    case TokenType::Identifier:
      tp = PapyrusType::Unresolved(expectConsumeIdent());
      break;

    default:
      fatalError("Expected a type!");
  }

  if (cur.type == TokenType::LSquare && peekToken().type == TokenType::RSquare) {
    consume();
    expectConsume(TokenType::RSquare);
    return PapyrusType::Array(new PapyrusType(tp));
  }
  return tp;
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

PapyrusUserFlags PapyrusParser::maybeConsumeUserFlags(PapyrusUserFlags validFlags) {
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
