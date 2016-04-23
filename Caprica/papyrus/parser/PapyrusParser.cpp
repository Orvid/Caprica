#include <papyrus/parser/PapyrusParser.h>

#include <vector>

#include <boost/filesystem.hpp>

#include <common/CaselessStringComparer.h>
#include <common/FSUtils.h>

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
#include <papyrus/statements/PapyrusBreakStatement.h>
#include <papyrus/statements/PapyrusContinueStatement.h>
#include <papyrus/statements/PapyrusDeclareStatement.h>
#include <papyrus/statements/PapyrusDoWhileStatement.h>
#include <papyrus/statements/PapyrusExpressionStatement.h>
#include <papyrus/statements/PapyrusForStatement.h>
#include <papyrus/statements/PapyrusForEachStatement.h>
#include <papyrus/statements/PapyrusIfStatement.h>
#include <papyrus/statements/PapyrusReturnStatement.h>
#include <papyrus/statements/PapyrusSwitchStatement.h>
#include <papyrus/statements/PapyrusWhileStatement.h>


namespace caprica { namespace papyrus { namespace parser {

PapyrusScript* PapyrusParser::parseScript() {
  auto script = new PapyrusScript();
  script->sourceFileName = FSUtils::canonical(filename).string();
  script->objects.push_back(parseObject(script));
  return script;
}

static bool doesScriptNameMatchNextPartOfDir(const boost::filesystem::path curPath, const std::string curName) {
  auto idx = curName.find_last_of(':');
  if (idx != std::string::npos) {
    auto namePiece = curName.substr(idx + 1);
    auto basePath = boost::filesystem::basename(curPath);
    if (_stricmp(namePiece.c_str(), basePath.c_str()))
      return false;
    return doesScriptNameMatchNextPartOfDir(curPath.parent_path(), curName.substr(0, idx));
  }
  return !_stricmp(boost::filesystem::basename(curPath).c_str(), curName.c_str());
}

PapyrusObject* PapyrusParser::parseObject(PapyrusScript* script) {
  auto loc = cur.location;
  maybeConsumeEOLs();

  expectConsume(TokenType::kScriptName);
  auto name = expectConsumeIdent();
  if (!doesScriptNameMatchNextPartOfDir(script->sourceFileName, name))
    CapricaError::error(cur.location, "The script name '%s' must match the name of the file '%s'!", name.c_str(), boost::filesystem::basename(script->sourceFileName).c_str());

  PapyrusObject* obj;
  if (maybeConsume(TokenType::kExtends)) {
    auto eLoc = cur.location;
    obj = new PapyrusObject(loc, PapyrusType::Unresolved(eLoc, expectConsumeIdent()));
  } else {
    obj = new PapyrusObject(loc, PapyrusType::None(cur.location));
  }
  obj->name = name;
  obj->userFlags = maybeConsumeUserFlags(CapricaUserFlagsDefinition::ValidLocations::Script);
  expectConsumeEOLs();
  obj->documentationString = maybeConsumeDocString();

  while (cur.type != TokenType::END) {
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

      case TokenType::kGroup:
        consume();
        obj->propertyGroups.push_back(parsePropertyGroup(script, obj));
        break;

      case TokenType::kCustomEvent: {
        consume();
        auto ce = new PapyrusCustomEvent(cur.location);
        ce->name = expectConsumeIdent();
        obj->customEvents.push_back(ce);
        expectConsumeEOLs();
        break;
      }

      case TokenType::kEvent:
        consume();
        obj->getRootState()->functions.push_back(parseFunction(script, obj, obj->getRootState(), PapyrusType::None(cur.location), TokenType::kEndEvent));
        break;
      case TokenType::kFunction:
        consume();
        obj->getRootState()->functions.push_back(parseFunction(script, obj, obj->getRootState(), PapyrusType::None(cur.location), TokenType::kEndFunction));
        break;

      case TokenType::kBool:
      case TokenType::kFloat:
      case TokenType::kInt:
      case TokenType::kString:
      case TokenType::kVar:
      case TokenType::Identifier:
      {
        auto tp = expectConsumePapyrusType();
        if (cur.type == TokenType::kFunction) {
          consume();
          obj->getRootState()->functions.push_back(parseFunction(script, obj, obj->getRootState(), tp, TokenType::kEndFunction));
        } else if (cur.type == TokenType::kProperty) {
          consume();
          obj->getRootPropertyGroup()->properties.push_back(parseProperty(script, obj, tp));
        } else {
          obj->variables.push_back(parseVariable(script, obj, tp));
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
    switch (cur.type) {
      case TokenType::kEndStruct:
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
        auto m = parseStructMember(script, object, struc, tp);
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

PapyrusStructMember* PapyrusParser::parseStructMember(PapyrusScript* script, PapyrusObject* object, PapyrusStruct* struc, PapyrusType tp) {
  auto mem = new PapyrusStructMember(cur.location, tp, struc);
  mem->name = expectConsumeIdent();

  // Needed because None is a valid default value, and we shouldn't
  // be erroring on it.
  bool hadDefault = false;
  if (maybeConsume(TokenType::Equal)) {
    hadDefault = true;
    mem->defaultValue = expectConsumePapyrusValue();
  }
  mem->userFlags = maybeConsumeUserFlags(CapricaUserFlagsDefinition::ValidLocations::StructMember);
  if (mem->isConst() && !hadDefault)
    CapricaError::error(cur.location, "A constant member must have a value!");
  expectConsumeEOLs();
  mem->documentationString = maybeConsumeDocString();
  return mem;
}

PapyrusPropertyGroup* PapyrusParser::parsePropertyGroup(PapyrusScript* script, PapyrusObject* object) {
  auto group = new PapyrusPropertyGroup(cur.location);
  group->name = expectConsumeIdent();
  group->userFlags = maybeConsumeUserFlags(CapricaUserFlagsDefinition::ValidLocations::PropertyGroup);
  expectConsumeEOLs();
  group->documentationComment = maybeConsumeDocString();

  while (true) {
    bool isConst = false;
    switch (cur.type) {
      case TokenType::kEndGroup:
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
  auto prop = new PapyrusProperty(cur.location, type, object);
  prop->name = expectConsumeIdent();

  bool isFullProp = true;
  bool hadDefaultValue = false;
  if (maybeConsume(TokenType::Equal)) {
    isFullProp = false;
    hadDefaultValue = true;
    prop->defaultValue = expectConsumePapyrusValue();
  }
  prop->userFlags = maybeConsumeUserFlags(CapricaUserFlagsDefinition::ValidLocations::Property);
  if (prop->isAuto() || prop->isReadOnly())
    isFullProp = false;
  if (prop->isReadOnly() && !hadDefaultValue)
    CapricaError::error(cur.location, "An AutoReadOnly property must have a value!");
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

PapyrusVariable* PapyrusParser::parseVariable(PapyrusScript* script, PapyrusObject* object, PapyrusType type) {
  auto var = new PapyrusVariable(cur.location, type, object);
  var->name = expectConsumeIdent();

  if (maybeConsume(TokenType::Equal)) {
    var->referenceState.isInitialized = true;
    var->defaultValue = expectConsumePapyrusValue();
  }
  var->userFlags = maybeConsumeUserFlags(CapricaUserFlagsDefinition::ValidLocations::Variable);
  if (var->isConst() && !var->referenceState.isInitialized)
    CapricaError::error(cur.location, "A constant variable must have a value!");
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
  if (endToken == TokenType::kEndEvent && maybeConsume(TokenType::Dot)) {
    func->functionType = PapyrusFunctionType::RemoteEvent;
    func->remoteEventParent = func->name;
    func->remoteEventName = expectConsumeIdent();
    func->name = "::remote_" + func->remoteEventParent + "_" + func->remoteEventName;
  }
  expectConsume(TokenType::LParen);

  if (cur.type != TokenType::RParen) {
    do {
      maybeConsume(TokenType::Comma);

      auto param = new PapyrusFunctionParameter(cur.location, expectConsumePapyrusType());
      param->name = expectConsumeIdent();
      if (maybeConsume(TokenType::Equal))
        param->defaultValue = expectConsumePapyrusValue();
      func->parameters.push_back(param);
    } while (cur.type == TokenType::Comma);
  }
  expectConsume(TokenType::RParen);

  func->userFlags = maybeConsumeUserFlags(CapricaUserFlagsDefinition::ValidLocations::Function);
  expectConsumeEOLs();
  func->documentationComment = maybeConsumeDocString();
  if (!func->isNative()) {
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
      auto ret = new statements::PapyrusReturnStatement(consumeLocation());
      if (cur.type != TokenType::EOL)
        ret->returnValue = parseExpression(func);
      expectConsumeEOLs();
      return ret;
    }

    case TokenType::kIf:
    {
      auto ret = new statements::PapyrusIfStatement(consumeLocation());
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
        return ret;
      }
    }

    case TokenType::kBreak:
    {
      auto ret = new statements::PapyrusBreakStatement(consumeLocation());
      expectConsumeEOLs();
      return ret;
    }

    case TokenType::kContinue:
    {
      auto ret = new statements::PapyrusContinueStatement(consumeLocation());
      expectConsumeEOLs();
      return ret;
    }

    case TokenType::kDo:
    {
      auto ret = new statements::PapyrusDoWhileStatement(consumeLocation());
      expectConsumeEOLs();
      while (cur.type != TokenType::kLoopWhile)
        ret->body.push_back(parseStatement(func));
      expectConsume(TokenType::kLoopWhile);
      ret->condition = parseExpression(func);
      expectConsumeEOLs();
      return ret;
    }

    case TokenType::kFor:
    {
      auto ret = new statements::PapyrusForStatement(consumeLocation());
      auto eLoc = cur.location;
      PapyrusIdentifier* ident{ nullptr };
      statements::PapyrusDeclareStatement* declStatement{ nullptr };
      if (peekTokenType() == TokenType::Identifier) {
        if (cur.type == TokenType::kAuto) {
          declStatement = new statements::PapyrusDeclareStatement(eLoc, PapyrusType::None(eLoc));
          declStatement->isAuto = true;
          expectConsume(TokenType::kAuto);
        } else {
          declStatement = new statements::PapyrusDeclareStatement(eLoc, expectConsumePapyrusType());
        }
        declStatement->name = expectConsumeIdent();
        ret->declareStatement = declStatement;
      } else {
        ident = new PapyrusIdentifier(PapyrusIdentifier::Unresolved(eLoc, expectConsumeIdent()));
        ret->iteratorVariable = ident;
      }
      expectConsume(TokenType::Equal);
      ret->initialValue = parseExpression(func);
      expectConsume(TokenType::kTo);
      ret->targetValue = parseExpression(func);
      if (maybeConsume(TokenType::kStep))
        ret->stepValue = parseExpression(func);
      expectConsumeEOLs();

      while (!maybeConsume(TokenType::kEndFor))
        ret->body.push_back(parseStatement(func));
      expectConsumeEOLs();

      return ret;
    }

    case TokenType::kForEach:
    {
      auto ret = new statements::PapyrusForEachStatement(consumeLocation());
      bool hadLParen = maybeConsume(TokenType::LParen);
      auto eLoc = cur.location;
      statements::PapyrusDeclareStatement* declStatement;
      if (cur.type == TokenType::kAuto) {
        declStatement = new statements::PapyrusDeclareStatement(eLoc, PapyrusType::None(eLoc));
        declStatement->isAuto = true;
        expectConsume(TokenType::kAuto);
      } else {
        declStatement = new statements::PapyrusDeclareStatement(eLoc, expectConsumePapyrusType());
      }
      declStatement->name = expectConsumeIdent();
      ret->declareStatement = declStatement;
      expectConsume(TokenType::kIn);
      ret->expressionToIterate = parseExpression(func);
      if (hadLParen)
        expectConsume(TokenType::RParen);
      expectConsumeEOLs();

      while (!maybeConsume(TokenType::kEndForEach))
        ret->body.push_back(parseStatement(func));
      expectConsumeEOLs();

      return ret;
    }

    case TokenType::kSwitch:
    {
      auto ret = new statements::PapyrusSwitchStatement(consumeLocation());
      ret->condition = parseExpression(func);
      expectConsumeEOLs();

      while (true) {
        switch (cur.type) {
          case TokenType::kCase:
          {
            consume();
            auto cond = expectConsumePapyrusValue();
            expectConsumeEOLs();
            std::vector<statements::PapyrusStatement*> curStatements{ };
            while (cur.type != TokenType::kCase && cur.type != TokenType::kEndSwitch && cur.type != TokenType::kDefault)
              curStatements.push_back(parseStatement(func));
            ret->caseBodies.push_back(std::make_pair(cond, curStatements));
            break;
          }

          case TokenType::kDefault:
          {
            if (ret->defaultStatements.size() > 0)
              CapricaError::error(cur.location, "The default case was already defined!");
            consume();
            expectConsumeEOLs();

            while (cur.type != TokenType::kCase && cur.type != TokenType::kEndSwitch && cur.type != TokenType::kDefault)
              ret->defaultStatements.push_back(parseStatement(func));
            break;
          }

          case TokenType::kEndSwitch:
            consume();
            expectConsumeEOLs();
            return ret;

          default:
            CapricaError::fatal(cur.location, "Unexpected token in switch body '%s'!", cur.prettyString().c_str());
        }
      }
    }

    case TokenType::kWhile:
    {
      auto ret = new statements::PapyrusWhileStatement(consumeLocation());
      ret->condition = parseExpression(func);
      expectConsumeEOLs();
      while (cur.type != TokenType::kEndWhile)
        ret->body.push_back(parseStatement(func));
      expectConsume(TokenType::kEndWhile);
      expectConsumeEOLs();
      return ret;
    }

    case TokenType::kAuto:
    {
      if (!CapricaConfig::enableLanguageExtensions)
        goto DefaultCase;

      auto eLoc = consumeLocation();
      auto ret = new statements::PapyrusDeclareStatement(eLoc, PapyrusType::None(eLoc));
      ret->isAuto = true;
      ret->name = expectConsumeIdent();
      expectConsume(TokenType::Equal);
      ret->initialValue = parseExpression(func);
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
      if (maybeConsume(TokenType::kConst))
        ret->isConst = true;
      expectConsumeEOLs();
      return ret;
    }

    DefaultCase:
    case TokenType::Identifier:
    default:
    {
      if (cur.type == TokenType::Identifier && (peekTokenType() == TokenType::Identifier ||  (peekTokenType() == TokenType::LSquare && peekTokenType(1) == TokenType::RSquare && peekTokenType(2) == TokenType::Identifier))) {
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
          auto assStat = new statements::PapyrusAssignStatement(consumeLocation());
          assStat->lValue = expr;
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
    auto binExpr = new expressions::PapyrusBinaryOpExpression(consumeLocation());
    binExpr->left = expr;
    binExpr->operation = expressions::PapyrusBinaryOperatorType::BooleanOr;
    binExpr->right = parseAndExpression(func);
    expr = binExpr;
  }
  return expr;
}

expressions::PapyrusExpression* PapyrusParser::parseAndExpression(PapyrusFunction* func) {
  auto expr = parseCmpExpression(func);
  while (cur.type == TokenType::BooleanAnd) {
    auto binExpr = new expressions::PapyrusBinaryOpExpression(consumeLocation());
    binExpr->left = expr;
    binExpr->operation = expressions::PapyrusBinaryOperatorType::BooleanAnd;
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
        auto binExpr = new expressions::PapyrusBinaryOpExpression(consumeLocation());
        binExpr->left = expr;
        binExpr->operation = op;
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
      case TokenType::Integer:
        if (cur.iValue >= 0)
          goto Return;
        goto DumbNegativesCommon;
      case TokenType::Float:
        if (cur.fValue >= 0)
          goto Return;
        goto DumbNegativesCommon;
      case TokenType::Minus:
        op = expressions::PapyrusBinaryOperatorType::Subtract;
        goto OperatorCommon;

      OperatorCommon:
      {
        auto binExpr = new expressions::PapyrusBinaryOpExpression(consumeLocation());
        binExpr->left = expr;
        binExpr->operation = op;
        binExpr->right = parseMultExpression(func);
        expr = binExpr;
        break;
      }

      DumbNegativesCommon:
      {
        if (!CapricaConfig::allowNegativeLiteralAsBinaryOp)
          goto Return;
        auto binExpr = new expressions::PapyrusBinaryOpExpression(cur.location);
        binExpr->left = expr;
        binExpr->operation = expressions::PapyrusBinaryOperatorType::Add;
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
        auto binExpr = new expressions::PapyrusBinaryOpExpression(consumeLocation());
        binExpr->left = expr;
        binExpr->operation = op;
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
      auto unExpr = new expressions::PapyrusUnaryOpExpression(consumeLocation());
      unExpr->operation = op;
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
    auto loc = consumeLocation();
    auto isExpr = new expressions::PapyrusIsExpression(loc, expectConsumePapyrusType());
    isExpr->innerExpression = expr;
    expr = isExpr;
  } else if (cur.type == TokenType::kAs) {
    auto loc = consumeLocation();
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
        auto maExpr = new expressions::PapyrusMemberAccessExpression(consumeLocation());
        maExpr->baseExpression = expr;
        maExpr->accessExpression = parseFuncOrIdExpression(func);

        if (cur.type == TokenType::LSquare) {
          auto aiExpr = new expressions::PapyrusArrayIndexExpression(consumeLocation());
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
    auto aiExpr = new expressions::PapyrusArrayIndexExpression(consumeLocation());
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

      return new expressions::PapyrusNewStructExpression(loc, tp);
    }

    default:
      return parseFuncOrIdExpression(func);
  }
}

expressions::PapyrusExpression* PapyrusParser::parseFuncOrIdExpression(PapyrusFunction* func) {
  switch (cur.type) {
    case TokenType::kLength:
      return new expressions::PapyrusArrayLengthExpression(consumeLocation());
    case TokenType::kParent:
      return new expressions::PapyrusParentExpression(consumeLocation(), func->parentObject->parentClass);
    case TokenType::kSelf:
    {
      auto selfExpr = new expressions::PapyrusSelfExpression(cur.location, PapyrusType::ResolvedObject(cur.location, func->parentObject));
      consume();
      return selfExpr;
    }
    case TokenType::Identifier:
    {
      if (peekTokenType() == TokenType::LParen) {
        auto eLoc = cur.location;
        auto fCallExpr = new expressions::PapyrusFunctionCallExpression(eLoc, PapyrusIdentifier::Unresolved(eLoc, expectConsumeIdent()));
        expectConsume(TokenType::LParen);

        if (cur.type != TokenType::RParen) {
          do {
            maybeConsume(TokenType::Comma);

            auto param = new expressions::PapyrusFunctionCallExpression::Parameter();
            if (cur.type == TokenType::Identifier && peekTokenType() == TokenType::Equal) {
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
      tp = PapyrusType::Bool(consumeLocation());
      break;
    case TokenType::kFloat:
      tp = PapyrusType::Float(consumeLocation());
      break;
    case TokenType::kInt:
      tp = PapyrusType::Int(consumeLocation());
      break;
    case TokenType::kCustomEventName:
    case TokenType::kScriptEventName:
    case TokenType::kString:
      tp = PapyrusType::String(consumeLocation());
      break;
    case TokenType::kVar:
      tp = PapyrusType::Var(consumeLocation());
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

  if (cur.type == TokenType::LSquare && peekTokenType() == TokenType::RSquare) {
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

PapyrusUserFlags PapyrusParser::maybeConsumeUserFlags(CapricaUserFlagsDefinition::ValidLocations location) {
  PapyrusUserFlags flags;
  while (cur.type != TokenType::END) {
    switch (cur.type) {
      case TokenType::kAuto:
        flags.isAuto = true;
        consume();
        break;
      case TokenType::kAutoReadOnly:
        flags.isAutoReadOnly = true;
        consume();
        break;
      case TokenType::kBetaOnly:
        flags.isBetaOnly = true;
        consume();
        break;
      case TokenType::kConst:
        flags.isConst = true;
        consume();
        break;
      case TokenType::kDebugOnly:
        flags.isDebugOnly = true;
        consume();
        break;
      case TokenType::kGlobal:
        flags.isGlobal = true;
        consume();
        break;
      case TokenType::kNative:
        flags.isNative = true;
        consume();
        break;
      case TokenType::Identifier:
      case TokenType::kDefault: {
        auto loc = cur.location;
        auto str = cur.sValue;
        if (cur.type == TokenType::kDefault)
          str = "default";

        if (!_stricmp(str.c_str(), "collapsed")) {
          PapyrusUserFlags newFlag;
          newFlag.data = (1ULL << 3) | (1ULL << 4);
          flags |= newFlag;
          consume();
          break;
        }

        auto flg = CapricaConfig::userFlagsDefinition.findFlag(loc, str);
        if ((flg.validLocations & location) != location && (location != CapricaUserFlagsDefinition::ValidLocations::Property || (flg.validLocations & CapricaUserFlagsDefinition::ValidLocations::Variable) != CapricaUserFlagsDefinition::ValidLocations::Variable))
          CapricaError::error(loc, "The flag '%s' is not valid in this location.", str.c_str());

        PapyrusUserFlags newFlag;
        newFlag.data = 1ULL << flg.flagNum;
        flags |= newFlag;
        consume();
        break;
      }
      default:
        goto Return;
    }
  }
Return:
  return flags;
}

}}}
