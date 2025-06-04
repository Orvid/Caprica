#pragma once

#include <string>

#include <common/CapricaReportingContext.h>
#include <common/CapricaUserFlagsDefinition.h>

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/PapyrusScript.h>
#include <papyrus/parser/PapyrusLexer.h>
#include <papyrus/statements/PapyrusStatement.h>

namespace caprica { namespace papyrus { namespace parser {

struct PapyrusParser final : private PapyrusLexer {
  explicit PapyrusParser(CapricaReportingContext& repCtx, const std::string& file, std::string_view data)
      : PapyrusLexer(repCtx, file, data) { }
  PapyrusParser(const PapyrusParser&) = delete;
  ~PapyrusParser() = default;

  PapyrusScript* parseScript();

private:
  PapyrusObject* parseObject(PapyrusScript* script);
  PapyrusState* parseState(PapyrusScript* script, PapyrusObject* object, bool isAuto);
  PapyrusStruct* parseStruct(PapyrusScript* script, PapyrusObject* object);
  PapyrusStructMember*
  parseStructMember(PapyrusScript* script, PapyrusObject* object, PapyrusStruct* struc, PapyrusType&& tp);
  PapyrusPropertyGroup* parsePropertyGroup(PapyrusScript* script, PapyrusObject* object);
  PapyrusProperty* parseProperty(PapyrusScript* script, PapyrusObject* object, PapyrusType&& type);
  PapyrusVariable* parseVariable(PapyrusScript* script, PapyrusObject* object, PapyrusType&& type);
  PapyrusFunction* parseFunction(
      PapyrusScript* script, PapyrusObject* object, PapyrusState* state, PapyrusType&& returnType, TokenType endToken);

  statements::PapyrusStatement* parseStatement(PapyrusFunction* func);

  expressions::PapyrusExpression* parseExpression(PapyrusFunction* func);
  expressions::PapyrusExpression* parseAndExpression(PapyrusFunction* func);
  expressions::PapyrusExpression* parseCmpExpression(PapyrusFunction* func);
  expressions::PapyrusExpression* parseAddExpression(PapyrusFunction* func);
  expressions::PapyrusExpression* parseMultExpression(PapyrusFunction* func);
  expressions::PapyrusExpression* parseUnaryExpression(PapyrusFunction* func);
  expressions::PapyrusExpression* parseCastExpression(PapyrusFunction* func);
  expressions::PapyrusExpression* parseDotExpression(PapyrusFunction* func);
  expressions::PapyrusExpression* parseArrayExpression(PapyrusFunction* func);
  expressions::PapyrusExpression* parseAtomExpression(PapyrusFunction* func);
  expressions::PapyrusExpression* parseFuncOrIdExpression(PapyrusFunction* func);

  PapyrusType expectConsumePapyrusType();
  PapyrusValue expectConsumePapyrusValue();
  identifier_ref expectConsumeKeywordOrIdentRef();
  PapyrusUserFlags maybeConsumeUserFlags(CapricaUserFlagsDefinition::ValidLocations location);

  ALWAYS_INLINE
  void expect(TokenType tp) {
    if (cur.type != tp) {
      if (tp == TokenType::EOL && cur.type == TokenType::END)
        return;
      reportingContext.fatal(cur.location,
                             "Syntax error! Expected '{}' got '{}'.",
                             Token::prettyTokenType(tp),
                             cur.prettyString());
    }
  }

  ALWAYS_INLINE
  void expectConsume(TokenType tp) {
    expect(tp);
    consume();
  }

  ALWAYS_INLINE
  bool maybeConsume(TokenType tp) {
    if (cur.type == tp) {
      consume();
      return true;
    }
    return false;
  }

  ALWAYS_INLINE
  void maybeConsumeEOLs() {
    while (maybeConsume(TokenType::EOL)) {
    }
  }

  ALWAYS_INLINE
  void expectConsumeEOLs() {
    expectConsume(TokenType::EOL);
    maybeConsumeEOLs();
  }

  ALWAYS_INLINE
  identifier_ref expectConsumeIdentRef() {
    expect(TokenType::Identifier);
    auto str = cur.val.s;
    consume();
    return str;
  }

  ALWAYS_INLINE
  identifier_ref maybeConsumeDocStringRef() {
    if (cur.type == TokenType::DocComment) {
      auto str = cur.val.s;
      consume();
      expectConsumeEOLs();
      return str;
    }
    maybeConsumeEOLs();
    return "";
  }
};

}}}
