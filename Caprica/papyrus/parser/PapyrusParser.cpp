#include <papyrus/parser/PapyrusParser.h>

#include <filesystem>
#include <vector>

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
#include <papyrus/statements/PapyrusForEachStatement.h>
#include <papyrus/statements/PapyrusForStatement.h>
#include <papyrus/statements/PapyrusGuardStatement.h>
#include <papyrus/statements/PapyrusIfStatement.h>
#include <papyrus/statements/PapyrusReturnStatement.h>
#include <papyrus/statements/PapyrusSwitchStatement.h>
#include <papyrus/statements/PapyrusTryGuardStatement.h>
#include <papyrus/statements/PapyrusWhileStatement.h>

namespace caprica { namespace papyrus { namespace parser {

static const caseless_unordered_identifier_ref_set skyrim_native_classes = {
  "action",
  "activator",
  "activemagiceffect",
  "actor",
  "actorbase",
  "alias",
  "ammo",
  "apparatus",
  "armor",
  "associationtype",
  "book",
  "cell",
  "class",
  "constructibleobject",
  "container",
  "debug",
  "door",
  "effectshader",
  "enchantment",
  "encounterzone",
  "explosion",
  "faction",
  "flora",
  "form",
  "formlist",
  "furniture",
  "game",
  "globalvariable",
  "hazard",
  "idle",
  "imagespacemodifier",
  "impactdataset",
  "ingredient",
  "key",
  "keyword",
  "leveledactor",
  "leveleditem",
  "leveledspell",
  "light",
  "location",
  "locationalias",
  "locationreftype",
  "magiceffect",
  "math",
  "message",
  "miscobject",
  "musictype",
  "objectreference",
  "outfit",
  "package",
  "perk",
  "potion",
  "projectile",
  "quest",
  "race",
  "referencealias",
  "scene",
  "scroll",
  "shaderparticlegeometry",
  "shout",
  "soulgem",
  "sound",
  "soundcategory",
  "spell",
  "static",
  "talkingactivator",
  "textureset",
  "topic",
  "topicinfo",
  "utility",
  "visualeffect",
  "voicetype",
  "weapon",
  "weather",
  "wordofpower",
  "worldspace",
  "__scriptobject" // TODO: Skyrim base object hack, address later.
};

PapyrusScript* PapyrusParser::parseScript() {
  auto script = alloc->make<PapyrusScript>();
  script->allocator = alloc;
  script->sourceFileName = FSUtils::canonical(filename);
  script->objects.push_back(parseObject(script));
  return script;
}

static bool doesScriptNameMatchNextPartOfDir(std::string_view curPath, const identifier_ref& curName) {
  auto idx = curName.rfind(':');
  if (idx != identifier_ref::npos) {
    auto namePiece = curName.substr(idx + 1);
    auto basePath = FSUtils::basenameAsRef(curPath);
    if (!pathEq(basePath, namePiece))
      return false;
    return doesScriptNameMatchNextPartOfDir(FSUtils::parentPathAsRef(curPath), curName.substr(0, idx));
  }
  return pathEq(FSUtils::basenameAsRef(curPath), curName);
}

PapyrusObject* PapyrusParser::parseObject(PapyrusScript* script) {
  auto loc = cur.location;
  maybeConsumeEOLs();

  expectConsume(TokenType::kScriptName);
  auto name = expectConsumeIdentRef();
  if (!doesScriptNameMatchNextPartOfDir(script->sourceFileName, name)) {
    reportingContext.error(cur.location,
                           "The script name '{}' must match the name of the file '{}'!",
                           name,
                           std::string(FSUtils::basenameAsRef(script->sourceFileName)));
  }

  PapyrusObject* obj;
  if (maybeConsume(TokenType::kExtends)) {
    auto eLoc = cur.location;
    obj = alloc->make<PapyrusObject>(loc, alloc, PapyrusType::Unresolved(eLoc, expectConsumeIdentRef()));
  } else if (conf::Papyrus::game >= GameID::Fallout4) {
    if (idEq(name, "ScriptObject"))
      obj = alloc->make<PapyrusObject>(loc, alloc, PapyrusType::None(cur.location));
    else
      obj = alloc->make<PapyrusObject>(loc, alloc, PapyrusType::Unresolved(loc, "ScriptObject"));
  } else {
    // TODO: SKYRIM HACK, address later
    // Skyrim does not have ScriptObject as its base type, but every
    // script still has a set of base methods and events. We need to load the fake __ScriptObject class.
    if (idEq(name, "__ScriptObject"))
      obj = alloc->make<PapyrusObject>(loc, alloc, PapyrusType::None(cur.location));
    else
      obj = alloc->make<PapyrusObject>(loc, alloc, PapyrusType::Unresolved(loc, "__ScriptObject"));
  }

  obj->setName(name);
  obj->userFlags = maybeConsumeUserFlags(CapricaUserFlagsDefinition::ValidLocations::Script);
  // Skyrim issue: None of the source scripts have the native flag set, including the native classes.
  // We need to do a lookup here and set it manually.
  if (conf::Papyrus::game == GameID::Skyrim && skyrim_native_classes.find(obj->name) != skyrim_native_classes.end())
    obj->userFlags.isNative = true;
  expectConsumeEOLs();
  obj->documentationString = maybeConsumeDocStringRef();

  while (cur.type != TokenType::END) {
    switch (cur.type) {
      case TokenType::kImport: {
        consume();
        auto eLoc = cur.location;
        obj->imports.emplace_back(eLoc, expectConsumeIdentRef());
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

      case TokenType::kGuard: {
        consume();
        auto guard = alloc->make<PapyrusGuard>(cur.location, obj);
        guard->name = expectConsumeIdentRef();
        expectConsumeEOLs();
        obj->guards.push_back(guard);
        break;
      }
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
        auto ce = alloc->make<PapyrusCustomEvent>(cur.location);
        ce->parentObject = obj;
        ce->name = expectConsumeIdentRef();
        obj->customEvents.push_back(ce);
        expectConsumeEOLs();
        break;
      }

      case TokenType::kEvent: {
        consume();
        auto f = parseFunction(script, obj, obj->getRootState(), PapyrusType::None(cur.location), TokenType::kEndEvent);
        obj->getRootState()->functions.emplace(f->name, f);
        break;
      }
      case TokenType::kFunction: {
        consume();
        auto f =
            parseFunction(script, obj, obj->getRootState(), PapyrusType::None(cur.location), TokenType::kEndFunction);
        obj->getRootState()->functions.emplace(f->name, f);
        break;
      }

      case TokenType::kBool:
      case TokenType::kFloat:
      case TokenType::kInt:
      case TokenType::kString:
      case TokenType::kVar:
      case TokenType::Identifier: {
        auto tp = expectConsumePapyrusType();
        if (cur.type == TokenType::kFunction) {
          consume();
          auto f = parseFunction(script, obj, obj->getRootState(), std::move(tp), TokenType::kEndFunction);
          obj->getRootState()->functions.emplace(f->name, f);
        } else if (cur.type == TokenType::kProperty) {
          consume();
          obj->getRootPropertyGroup()->properties.push_back(parseProperty(script, obj, std::move(tp)));
        } else {
          obj->variables.push_back(parseVariable(script, obj, std::move(tp)));
        }
        break;
      }

      default:
        reportingContext.fatal(cur.location, "Unexpected token '{}'!", cur.prettyString());
    }
  }

  return obj;
}

PapyrusState* PapyrusParser::parseState(PapyrusScript* script, PapyrusObject* object, bool isAuto) {
  auto state = alloc->make<PapyrusState>(cur.location);
  state->name = expectConsumeIdentRef();
  if (isAuto) {
    if (object->autoState != nullptr) {
      reportingContext.error(cur.location,
                             "Only one state can be declared auto. '{}' was already declared as the auto state.",
                             object->autoState->name);
    }
    object->autoState = state;
  }
  expectConsumeEOLs();

  while (true) {
    switch (cur.type) {
      case TokenType::kEndState:
        consume();
        expectConsumeEOLs();
        goto Return;

      case TokenType::kEvent: {
        consume();
        auto f = parseFunction(script, object, state, PapyrusType::None(cur.location), TokenType::kEndEvent);
        state->functions.emplace(f->name, f);
        break;
      }
      case TokenType::kFunction: {
        consume();
        auto f = parseFunction(script, object, state, PapyrusType::None(cur.location), TokenType::kEndFunction);
        state->functions.emplace(f->name, f);
        break;
      }

      case TokenType::kBool:
      case TokenType::kFloat:
      case TokenType::kInt:
      case TokenType::kString:
      case TokenType::kVar:
      case TokenType::Identifier: {
        auto tp = expectConsumePapyrusType();
        expectConsume(TokenType::kFunction);
        auto f = parseFunction(script, object, state, std::move(tp), TokenType::kEndFunction);
        state->functions.emplace(f->name, f);
        break;
      }

      default:
        reportingContext.fatal(cur.location, "Expected an event or function, got '{}'!", cur.prettyString());
    }
  }

Return:
  return state;
}

PapyrusStruct* PapyrusParser::parseStruct(PapyrusScript* script, PapyrusObject* object) {
  auto struc = alloc->make<PapyrusStruct>(cur.location);
  struc->parentObject = object;
  struc->name = expectConsumeIdentRef();
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
        struc->members.push_back(parseStructMember(script, object, struc, expectConsumePapyrusType()));
        break;

      default:
        reportingContext.fatal(cur.location, "Unexpected token '{}' while parsing struct!", cur.prettyString());
    }
  }

Return:
  return struc;
}

PapyrusStructMember*
PapyrusParser::parseStructMember(PapyrusScript*, PapyrusObject*, PapyrusStruct* struc, PapyrusType&& tp) {
  auto mem = alloc->make<PapyrusStructMember>(cur.location, std::move(tp), struc);
  mem->name = expectConsumeIdentRef();

  // Needed because None is a valid default value, and we shouldn't
  // be erroring on it.
  bool hadDefault = false;
  if (maybeConsume(TokenType::Equal)) {
    hadDefault = true;
    mem->defaultValue = expectConsumePapyrusValue();
  }
  mem->userFlags = maybeConsumeUserFlags(CapricaUserFlagsDefinition::ValidLocations::StructMember);
  if (mem->isConst() && !hadDefault)
    reportingContext.error(cur.location, "A constant member must have a value!");
  expectConsumeEOLs();
  mem->documentationString = maybeConsumeDocStringRef();
  return mem;
}

PapyrusPropertyGroup* PapyrusParser::parsePropertyGroup(PapyrusScript* script, PapyrusObject* object) {
  auto group = alloc->make<PapyrusPropertyGroup>(cur.location);
  group->name = expectConsumeIdentRef();
  group->userFlags = maybeConsumeUserFlags(CapricaUserFlagsDefinition::ValidLocations::PropertyGroup);
  expectConsumeEOLs();
  group->documentationComment = maybeConsumeDocStringRef();

  while (true) {
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
      case TokenType::Identifier: {
        auto tp = expectConsumePapyrusType();
        expectConsume(TokenType::kProperty);
        group->properties.push_back(parseProperty(script, object, std::move(tp)));
        break;
      }

      default:
        reportingContext.fatal(cur.location,
                               "Unexpected token '{}' while parsing property group!",
                               cur.prettyString());
    }
  }

Return:
  return group;
}

PapyrusProperty* PapyrusParser::parseProperty(PapyrusScript* script, PapyrusObject* object, PapyrusType&& type) {
  auto prop = alloc->make<PapyrusProperty>(cur.location, std::move(type), object);
  prop->name = expectConsumeIdentRef();

  bool isFullProp = true;
  bool hadDefaultValue = false;
  if (maybeConsume(TokenType::Equal)) {
    isFullProp = false;
    hadDefaultValue = true;
    prop->defaultValue = expectConsumePapyrusValue();
  }
  prop->userFlags = maybeConsumeUserFlags(CapricaUserFlagsDefinition::ValidLocations::Property);
  if (prop->isAuto() || prop->isAutoReadOnly())
    isFullProp = false;
  if (prop->isAutoReadOnly() && !hadDefaultValue)
    reportingContext.error(cur.location, "An AutoReadOnly property must have a value!");
  expectConsumeEOLs();
  prop->documentationComment = maybeConsumeDocStringRef();

  if (!isFullProp) {
    prop->buildAutoVarName(alloc);
  } else {
    for (int i = 0; i < 2; i++) {
      switch (cur.type) {
        case TokenType::kFunction:
          if (prop->writeFunction)
            reportingContext.error(cur.location, "The set function for this property has already been defined!");
          consume();
          prop->writeFunction =
              parseFunction(script, object, nullptr, PapyrusType::None(cur.location), TokenType::kEndFunction);
          if (!prop->writeFunction)
            CapricaReportingContext::logicalFatal("Somehow failed while parsing the property setter!");
          prop->writeFunction->functionType = PapyrusFunctionType::Setter;
          if (!idEq(prop->writeFunction->name, "set"))
            reportingContext.error(cur.location, "The set function must be named \"Set\"!");
          if (prop->writeFunction->parameters.size() != 1)
            reportingContext.error(cur.location, "The set function must have a single parameter!");
          if (prop->writeFunction->parameters.front()->type != prop->type)
            reportingContext.error(cur.location, "The set function's parameter must be the same type as the property!");
          break;

        case TokenType::kBool:
        case TokenType::kFloat:
        case TokenType::kInt:
        case TokenType::kString:
        case TokenType::kVar:
        case TokenType::Identifier: {
          if (prop->readFunction)
            reportingContext.error(cur.location, "The get function for this property has already been defined!");
          auto tp = expectConsumePapyrusType();
          if (tp != prop->type) {
            reportingContext.error(cur.location,
                                   "The return type of the get function must be the same as the property!");
          }
          expectConsume(TokenType::kFunction);
          prop->readFunction = parseFunction(script, object, nullptr, std::move(tp), TokenType::kEndFunction);
          if (!prop->readFunction)
            CapricaReportingContext::logicalFatal("Somehow failed while parsing the property getter!");
          prop->readFunction->functionType = PapyrusFunctionType::Getter;
          if (!idEq(prop->readFunction->name, "get"))
            reportingContext.error(cur.location, "The get function must be named \"Get\"!");
          if (prop->readFunction->parameters.size() != 0)
            reportingContext.error(cur.location, "The get function cannot have parameters!");
          break;
        }

        case TokenType::kEndProperty:
          break;

        default:
          reportingContext.fatal(cur.location,
                                 "Expected the get/set functions of a full property, got '{}'!",
                                 cur.prettyString());
      }
    }

    expectConsume(TokenType::kEndProperty);
    expectConsumeEOLs();
  }

  return prop;
}

PapyrusVariable* PapyrusParser::parseVariable(PapyrusScript*, PapyrusObject* object, PapyrusType&& type) {
  auto var = alloc->make<PapyrusVariable>(cur.location, std::move(type), object);
  if (conf::Papyrus::game != GameID::Skyrim)
    var->name = expectConsumeIdentRef();
  else
    var->name = expectConsumeIdentRef();
  if (maybeConsume(TokenType::Equal)) {
    var->referenceState.isInitialized = true;
    var->defaultValue = expectConsumePapyrusValue();
  }
  var->userFlags = maybeConsumeUserFlags(CapricaUserFlagsDefinition::ValidLocations::Variable);
  if (var->isConst() && !var->referenceState.isInitialized)
    reportingContext.error(cur.location, "A constant variable must have a value!");
  expectConsumeEOLs();
  return var;
}

PapyrusFunction* PapyrusParser::parseFunction(
    PapyrusScript*, PapyrusObject* object, PapyrusState*, PapyrusType&& returnType, TokenType endToken) {
  auto func = alloc->make<PapyrusFunction>(cur.location, std::move(returnType));
  if (endToken == TokenType::kEndFunction)
    func->functionType = PapyrusFunctionType::Function;
  else if (endToken == TokenType::kEndEvent)
    func->functionType = PapyrusFunctionType::Event;
  else
    CapricaReportingContext::logicalFatal("Unknown end token for a parseFunction call!");
  func->parentObject = object;
  func->name = expectConsumeIdentRef();
  if (endToken == TokenType::kEndEvent && maybeConsume(TokenType::Dot)) {
    func->functionType = PapyrusFunctionType::RemoteEvent;
    func->remoteEventParent = func->name;
    func->remoteEventName = expectConsumeIdentRef();
    func->name = alloc->allocateIdentifier("::remote_" + func->remoteEventParent.to_string() + "_" +
                                           func->remoteEventName.to_string());
  }
  expectConsume(TokenType::LParen);

  if (cur.type != TokenType::RParen) {
    do {
      maybeConsume(TokenType::Comma);

      auto param =
          alloc->make<PapyrusFunctionParameter>(cur.location, func->parameters.size(), expectConsumePapyrusType());
      param->name = expectConsumeKeywordOrIdentRef();
      if (maybeConsume(TokenType::Equal))
        param->defaultValue = expectConsumePapyrusValue();
      func->parameters.push_back(param);
    } while (cur.type == TokenType::Comma);
  }
  expectConsume(TokenType::RParen);

  func->userFlags = maybeConsumeUserFlags(CapricaUserFlagsDefinition::ValidLocations::Function);
  expectConsumeEOLs();
  func->documentationComment = maybeConsumeDocStringRef();
  if (!func->isNative()) {
    while (cur.type != endToken && cur.type != TokenType::END)
      func->statements.push_back(parseStatement(func));

    if (cur.type == TokenType::END) {
      reportingContext.error(cur.location, "Unexpected EOF in state body!");
      return func;
    }
    consume();
    expectConsumeEOLs();
  }

  return func;
}

statements::PapyrusStatement* PapyrusParser::parseStatement(PapyrusFunction* func) {
  switch (cur.type) {
    case TokenType::kReturn: {
      auto ret = alloc->make<statements::PapyrusReturnStatement>(consumeLocation());
      if (cur.type != TokenType::EOL)
        ret->returnValue = parseExpression(func);
      expectConsumeEOLs();
      return ret;
    }

    case TokenType::kGuard: {
      reportingContext.warning_W6002_Experimental_Syntax_Lock(cur.location);

      auto ret = alloc->make<statements::PapyrusGuardStatement>(consumeLocation());

      if (cur.type == TokenType::EOL)
        reportingContext.fatal(cur.location, "Syntax error: Guard statement with no guards specified!");
      else if (cur.type != TokenType::Identifier)
        reportingContext.fatal(cur.location, "Syntax error: Incorrect Guard Statement");
      size_t idx = 0;
      do {
        maybeConsume(TokenType::Comma);
        auto guard = alloc->make<PapyrusLockParameter>(cur.location, idx++);
        guard->name = expectConsumeIdentRef();
        ret->lockParams.push_back(guard);
      } while (cur.type == TokenType::Comma);
      expectConsumeEOLs();
      while (!maybeConsume(TokenType::kEndGuard))
        ret->body.push_back(parseStatement(func));
      expectConsumeEOLs();
      return ret;
    }
    case TokenType::kTryGuard: {
      reportingContext.warning_W6003_Experimental_Syntax_TryLock(cur.location);
      auto ret = alloc->make<statements::PapyrusTryGuardStatement>(consumeLocation());
      if (cur.type == TokenType::EOL)
        reportingContext.fatal(cur.location, "Syntax error: TryGuard statement with no guards specified!");
      else if (cur.type != TokenType::Identifier)
        reportingContext.fatal(cur.location, "Syntax error: Incorrect TryGuard Statement");
      size_t idx = 0;
      do {
        maybeConsume(TokenType::Comma);
        auto guard = alloc->make<PapyrusLockParameter>(cur.location, idx++);
        guard->name = expectConsumeIdentRef();
        ret->lockParams.push_back(guard);
      } while (cur.type == TokenType::Comma);
      expectConsumeEOLs();
      while (!maybeConsume(TokenType::kEndGuard))
        ret->body.push_back(parseStatement(func));
      expectConsumeEOLs();
      return ret;
    }

    case TokenType::kIf: {
      auto ret = alloc->make<statements::PapyrusIfStatement>(consumeLocation());
      while (true) {
        auto cond = parseExpression(func);
        expectConsumeEOLs();
        IntrusiveLinkedList<statements::PapyrusStatement> curStatements {};
        while (cur.type != TokenType::kElseIf && cur.type != TokenType::kElse && cur.type != TokenType::kEndIf)
          curStatements.push_back(parseStatement(func));
        ret->ifBodies.push_back(alloc->make<statements::PapyrusIfStatement::IfBody>(cond, std::move(curStatements)));
        if (cur.type == TokenType::kElseIf) {
          consume();
          continue;
        }
        if (cur.type == TokenType::kElse) {
          consume();
          expectConsumeEOLs();
          while (cur.type != TokenType::kEndIf)
            ret->elseStatements.push_back(parseStatement(func));
        }
        expectConsume(TokenType::kEndIf);
        expectConsumeEOLs();
        return ret;
      }
    }

    case TokenType::kBreak: {
      auto ret = alloc->make<statements::PapyrusBreakStatement>(consumeLocation());
      expectConsumeEOLs();
      return ret;
    }

    case TokenType::kContinue: {
      auto ret = alloc->make<statements::PapyrusContinueStatement>(consumeLocation());
      expectConsumeEOLs();
      return ret;
    }

    case TokenType::kDo: {
      auto ret = alloc->make<statements::PapyrusDoWhileStatement>(consumeLocation());
      expectConsumeEOLs();
      while (cur.type != TokenType::kLoopWhile)
        ret->body.push_back(parseStatement(func));
      expectConsume(TokenType::kLoopWhile);
      ret->condition = parseExpression(func);
      expectConsumeEOLs();
      return ret;
    }

    case TokenType::kFor: {
      auto ret = alloc->make<statements::PapyrusForStatement>(consumeLocation());
      auto eLoc = cur.location;
      PapyrusIdentifier* ident { nullptr };
      statements::PapyrusDeclareStatement* declStatement { nullptr };
      if (peekTokenType() == TokenType::Identifier) {
        if (cur.type == TokenType::kAuto) {
          declStatement = alloc->make<statements::PapyrusDeclareStatement>(eLoc, PapyrusType::None(eLoc));
          declStatement->isAuto = true;
          expectConsume(TokenType::kAuto);
        } else {
          declStatement = alloc->make<statements::PapyrusDeclareStatement>(eLoc, expectConsumePapyrusType());
        }
        declStatement->name = expectConsumeIdentRef();
        ret->declareStatement = declStatement;
      } else {
        ident = alloc->make<PapyrusIdentifier>(PapyrusIdentifier::Unresolved(eLoc, expectConsumeIdentRef()));
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

    case TokenType::kForEach: {
      auto ret = alloc->make<statements::PapyrusForEachStatement>(consumeLocation());
      bool hadLParen = maybeConsume(TokenType::LParen);
      auto eLoc = cur.location;
      statements::PapyrusDeclareStatement* declStatement;
      if (cur.type == TokenType::kAuto) {
        declStatement = alloc->make<statements::PapyrusDeclareStatement>(eLoc, PapyrusType::None(eLoc));
        declStatement->isAuto = true;
        expectConsume(TokenType::kAuto);
      } else {
        declStatement = alloc->make<statements::PapyrusDeclareStatement>(eLoc, expectConsumePapyrusType());
      }
      declStatement->name = expectConsumeIdentRef();
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

    case TokenType::kSwitch: {
      auto ret = alloc->make<statements::PapyrusSwitchStatement>(consumeLocation());
      ret->condition = parseExpression(func);
      expectConsumeEOLs();

      while (true) {
        switch (cur.type) {
          case TokenType::kCase: {
            consume();
            auto cond = expectConsumePapyrusValue();
            expectConsumeEOLs();
            IntrusiveLinkedList<statements::PapyrusStatement> curStatements {};
            while (cur.type != TokenType::kCase && cur.type != TokenType::kEndSwitch && cur.type != TokenType::kDefault)
              curStatements.push_back(parseStatement(func));
            ret->caseBodies.push_back(
                alloc->make<statements::PapyrusSwitchStatement::CaseBody>(std::move(cond), std::move(curStatements)));
            break;
          }

          case TokenType::kDefault: {
            if (ret->defaultStatements.size() > 0)
              reportingContext.error(cur.location, "The default case was already defined!");
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
            reportingContext.fatal(cur.location, "Unexpected token in switch body '{}'!", cur.prettyString());
        }
      }
    }

    case TokenType::kWhile: {
      auto ret = alloc->make<statements::PapyrusWhileStatement>(consumeLocation());
      ret->condition = parseExpression(func);
      expectConsumeEOLs();
      while (cur.type != TokenType::kEndWhile)
        ret->body.push_back(parseStatement(func));
      expectConsume(TokenType::kEndWhile);
      expectConsumeEOLs();
      return ret;
    }

    case TokenType::kAuto: {
      if (!conf::Papyrus::enableLanguageExtensions)
        goto DefaultCase;

      auto eLoc = consumeLocation();
      auto ret = alloc->make<statements::PapyrusDeclareStatement>(eLoc, PapyrusType::None(eLoc));
      ret->isAuto = true;
      ret->name = expectConsumeIdentRef();
      expectConsume(TokenType::Equal);
      ret->initialValue = parseExpression(func);
      expectConsumeEOLs();
      return ret;
    }

    case TokenType::kBool:
    case TokenType::kFloat:
    case TokenType::kInt:
    case TokenType::kString:
    case TokenType::kVar: {
      auto eLoc = cur.location;
      auto ret = alloc->make<statements::PapyrusDeclareStatement>(eLoc, expectConsumePapyrusType());
      ret->name = expectConsumeIdentRef();
      if (maybeConsume(TokenType::Equal))
        ret->initialValue = parseExpression(func);
      if (maybeConsume(TokenType::kConst))
        ret->isConst = true;
      expectConsumeEOLs();
      return ret;
    }

    DefaultCase:
    case TokenType::Identifier:
    default: {
      if (cur.type == TokenType::Identifier &&
          (peekTokenType() == TokenType::Identifier ||
           (peekTokenType() == TokenType::LSquare && peekTokenType(1) == TokenType::RSquare &&
            peekTokenType(2) == TokenType::Identifier))) {
        auto eLoc = cur.location;
        auto ret = alloc->make<statements::PapyrusDeclareStatement>(eLoc, expectConsumePapyrusType());
        ret->name = expectConsumeIdentRef();
        if (maybeConsume(TokenType::Equal))
          ret->initialValue = parseExpression(func);
        if (maybeConsume(TokenType::kConst))
          ret->isConst = true;
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
        AssignStatementCommon : {
          auto assStat = alloc->make<statements::PapyrusAssignStatement>(consumeLocation());
          assStat->lValue = expr;
          assStat->operation = op;
          assStat->rValue = parseExpression(func);
          expectConsumeEOLs();
          return assStat;
        }

        default: {
          auto exprStat = alloc->make<statements::PapyrusExpressionStatement>(expr->location);
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
    auto binExpr = alloc->make<expressions::PapyrusBinaryOpExpression>(consumeLocation());
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
    auto binExpr = alloc->make<expressions::PapyrusBinaryOpExpression>(consumeLocation());
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

      OperatorCommon : {
        auto binExpr = alloc->make<expressions::PapyrusBinaryOpExpression>(consumeLocation());
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
        if (cur.val.i >= 0)
          goto Return;
        goto DumbNegativesCommon;
      case TokenType::Float:
        if (cur.val.f >= 0)
          goto Return;
        goto DumbNegativesCommon;
      case TokenType::Minus:
        op = expressions::PapyrusBinaryOperatorType::Subtract;
        goto OperatorCommon;

      OperatorCommon : {
        auto binExpr = alloc->make<expressions::PapyrusBinaryOpExpression>(consumeLocation());
        binExpr->left = expr;
        binExpr->operation = op;
        binExpr->right = parseMultExpression(func);
        expr = binExpr;
        break;
      }

      DumbNegativesCommon : {
        if (!conf::Papyrus::allowNegativeLiteralAsBinaryOp)
          goto Return;
        auto binExpr = alloc->make<expressions::PapyrusBinaryOpExpression>(cur.location);
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

      OperatorCommon : {
        auto binExpr = alloc->make<expressions::PapyrusBinaryOpExpression>(consumeLocation());
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

    OperatorCommon : {
      auto unExpr = alloc->make<expressions::PapyrusUnaryOpExpression>(consumeLocation());
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
    auto isExpr = alloc->make<expressions::PapyrusIsExpression>(loc, expectConsumePapyrusType());
    isExpr->innerExpression = expr;
    expr = isExpr;
  } else if (cur.type == TokenType::kAs) {
    auto loc = consumeLocation();
    auto castExpr = alloc->make<expressions::PapyrusCastExpression>(loc, expectConsumePapyrusType());
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
    case TokenType::kFalse: {
      auto eLoc = cur.location;
      return alloc->make<expressions::PapyrusLiteralExpression>(eLoc, expectConsumePapyrusValue());
    }

    default: {
      auto expr = parseArrayExpression(func);
      while (cur.type == TokenType::Dot) {
        auto maExpr = alloc->make<expressions::PapyrusMemberAccessExpression>(consumeLocation());
        maExpr->baseExpression = expr;
        maExpr->accessExpression = parseFuncOrIdExpression(func);

        if (cur.type == TokenType::LSquare) {
          auto aiExpr = alloc->make<expressions::PapyrusArrayIndexExpression>(consumeLocation());
          aiExpr->baseExpression = maExpr;
          aiExpr->indexExpression = parseExpression(func);
          expectConsume(TokenType::RSquare);
          expr = aiExpr;
        } else {
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
    auto aiExpr = alloc->make<expressions::PapyrusArrayIndexExpression>(consumeLocation());
    aiExpr->baseExpression = expr;
    aiExpr->indexExpression = parseExpression(func);
    expectConsume(TokenType::RSquare);
    return aiExpr;
  }
  return expr;
}

expressions::PapyrusExpression* PapyrusParser::parseAtomExpression(PapyrusFunction* func) {
  switch (cur.type) {
    case TokenType::LParen: {
      consume();
      auto expr = parseExpression(func);
      expectConsume(TokenType::RParen);
      return expr;
    }

    case TokenType::kNew: {
      auto loc = cur.location;
      consume();
      auto tp = expectConsumePapyrusType();
      if (maybeConsume(TokenType::LSquare)) {
        auto nArrExpr = alloc->make<expressions::PapyrusNewArrayExpression>(loc, std::move(tp));
        nArrExpr->lengthExpression = parseExpression(func);
        expectConsume(TokenType::RSquare);
        return nArrExpr;
      }

      return alloc->make<expressions::PapyrusNewStructExpression>(loc, std::move(tp));
    }

    default:
      return parseFuncOrIdExpression(func);
  }
}

expressions::PapyrusExpression* PapyrusParser::parseFuncOrIdExpression(PapyrusFunction* func) {
  switch (cur.type) {
    case TokenType::kLength:
      return alloc->make<expressions::PapyrusArrayLengthExpression>(consumeLocation());
    case TokenType::kParent:
      return alloc->make<expressions::PapyrusParentExpression>(consumeLocation(), func->parentObject->parentClass);
    case TokenType::kSelf: {
      auto selfExpr = alloc->make<expressions::PapyrusSelfExpression>(
          cur.location,
          PapyrusType::ResolvedObject(cur.location, func->parentObject));
      consume();
      return selfExpr;
    }
    case TokenType::Identifier: {
      if (peekTokenType() == TokenType::LParen) {
        auto eLoc = cur.location;
        auto fCallExpr = alloc->make<expressions::PapyrusFunctionCallExpression>(
            eLoc,
            PapyrusIdentifier::Unresolved(eLoc, expectConsumeIdentRef()));
        expectConsume(TokenType::LParen);

        if (cur.type != TokenType::RParen) {
          do {
            maybeConsume(TokenType::Comma);

            auto param = alloc->make<expressions::PapyrusFunctionCallExpression::Parameter>();
            if (cur.type == TokenType::Identifier && peekTokenType() == TokenType::Equal) {
              param->name = expectConsumeIdentRef();
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
        return alloc->make<expressions::PapyrusIdentifierExpression>(
            eLoc,
            PapyrusIdentifier::Unresolved(eLoc, expectConsumeIdentRef()));
      }
    }
    default:
      reportingContext.fatal(cur.location, "Unexpected token '{}'!", cur.prettyString());
  }
}

identifier_ref PapyrusParser::expectConsumeKeywordOrIdentRef() {
  if (!keywordCanBeIdentifier(cur.type)) {
    reportingContext.fatal(cur.location,
                          "Syntax error! Expected valid identifier, got '{}'.",
                          cur.prettyString());
  }

  identifier_ref finalId;

  if (cur.type != TokenType::Identifier) {
    const identifier_ref typeIdentifier = identifier_ref(PapyrusLexer::Token::prettyTokenType(cur.type));
    reportingContext.warning_W1006_Strict_Keyword_Identifiers(cur.location, typeIdentifier);
    finalId = typeIdentifier;
  } else {
    finalId = cur.val.s;
  }
  consume();
  return finalId;
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
    case TokenType::kString:
      tp = PapyrusType::String(consumeLocation());
      break;
    case TokenType::kVar:
      tp = PapyrusType::Var(consumeLocation());
      break;
    case TokenType::kCustomEventName:
      tp = PapyrusType::String(consumeLocation());
      tp.type = PapyrusType::Kind::CustomEventName;
      break;
    case TokenType::kScriptEventName:
      tp = PapyrusType::String(consumeLocation());
      tp.type = PapyrusType::Kind::ScriptEventName;
      break;
    case TokenType::Identifier: {
      auto eLoc = cur.location;
      tp = PapyrusType::Unresolved(eLoc, expectConsumeIdentRef());
      break;
    }

    default:
      reportingContext.fatal(cur.location, "Expected a type, got '{}'!", cur.prettyString());
  }

  if (cur.type == TokenType::LSquare && peekTokenType() == TokenType::RSquare) {
    consume();
    expectConsume(TokenType::RSquare);
    return PapyrusType::Array(tp.location, alloc->make<PapyrusType>(tp));
  }
  return tp;
}

PapyrusValue PapyrusParser::expectConsumePapyrusValue() {
  PapyrusValue val { cur.location };

  switch (cur.type) {
    case TokenType::Float:
      val.type = PapyrusValueType::Float;
      val.val.f = cur.val.f;
      consume();
      return val;
    case TokenType::Integer:
      val.type = PapyrusValueType::Integer;
      val.val.i = cur.val.i;
      consume();
      return val;
    case TokenType::String:
      val.type = PapyrusValueType::String;
      val.val.s = cur.val.s;
      consume();
      return val;
    case TokenType::kNone:
      val.type = PapyrusValueType::None;
      consume();
      return val;
    case TokenType::kTrue:
      val.type = PapyrusValueType::Bool;
      val.val.b = true;
      consume();
      return val;
    case TokenType::kFalse:
      val.type = PapyrusValueType::Bool;
      val.val.b = false;
      consume();
      return val;

    default:
      reportingContext.fatal(cur.location, "Expected a default value, got '{}'!", cur.prettyString());
  }
}

PapyrusUserFlags PapyrusParser::maybeConsumeUserFlags(CapricaUserFlagsDefinition::ValidLocations validLocs) {
  PapyrusUserFlags flags;
  while (cur.type != TokenType::END) {
    switch (cur.type) {
      case TokenType::kAuto:
        flags.isAuto = true;
        consume();
        break;
      case TokenType::kAutoReadOnly:
        flags.isAutoReadOnly = true;
        flags.isAuto = true;
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
        auto str = cur.val.s.to_string();
        if (cur.type == TokenType::kDefault)
          str = "default";

        auto& flg = conf::Papyrus::userFlagsDefinition.findFlag(reportingContext, loc, str);
        if (!flg.isValidOn(validLocs))
          reportingContext.error(loc, "The flag '{}' is not valid in this location.", str);

        PapyrusUserFlags newFlag;
        newFlag.data = flg.getData();
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
