#pragma once

#include <string>

#include <papyrus/PapyrusScript.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/parser/PapyrusLexer.h>
#include <papyrus/statements/PapyrusStatement.h>

namespace caprica { namespace papyrus { namespace parser {

struct PapyrusParser : private PapyrusLexer
{
  PapyrusParser(std::string file) : PapyrusLexer(file) { }
  ~PapyrusParser() = default;

  PapyrusScript* parseScript();
  
private:
  PapyrusObject* parseObject(PapyrusScript* script);
  PapyrusState* parseState(PapyrusScript* script, PapyrusObject* object, bool isAuto);
  PapyrusStruct* parseStruct(PapyrusScript* script, PapyrusObject* object);
  PapyrusStructMember* parseStructMember(PapyrusScript* script, PapyrusObject* object, PapyrusStruct* struc, bool isConst, PapyrusType tp);
  PapyrusPropertyGroup* parsePropertyGroup(PapyrusScript* script, PapyrusObject* object);
  PapyrusProperty* parseProperty(PapyrusScript* script, PapyrusObject* object, bool isConst, PapyrusType type);
  PapyrusVariable* parseVariable(PapyrusScript* script, PapyrusObject* object, bool isConst, PapyrusType type);
  PapyrusFunction* parseFunction(PapyrusScript* script, PapyrusObject* object, PapyrusState* state, PapyrusType returnType, TokenType endToken);

  statements::PapyrusStatement* parseStatement(PapyrusFunction* func);
  expressions::PapyrusExpression* parseExpression(PapyrusFunction* func);

  PapyrusType expectConsumePapyrusType();
  PapyrusValue expectConsumePapyrusValue();
  PapyrusUserFlags maybeConsumeUserFlags(PapyrusUserFlags validFlags);

  void expect(TokenType tp) {
    if (cur.type != tp) {
      if (tp == TokenType::EOL && cur.type == TokenType::END)
        return;
      fatalError("Unexpected token " + cur.prettyString() + "!");
    }
  }

  void expectConsume(TokenType tp) {
    expect(tp);
    consume();
  }

  bool maybeConsume(TokenType tp) {
    if (cur.type == tp) {
      consume();
      return true;
    }
    return false;
  }

  void maybeConsumeEOLs() {
    while (maybeConsume(TokenType::EOL)) { }
  }

  void expectConsumeEOLs() {
    expectConsume(TokenType::EOL);
    maybeConsumeEOLs();
  }

  std::string expectConsumeIdent() {
    expect(TokenType::Identifier);
    auto str = cur.sValue;
    consume();
    return str;
  }

  std::string maybeConsumeDocString() {
    if (cur.type == TokenType::DocComment) {
      auto str = cur.sValue;
      consume();
      expectConsumeEOLs();
      return str;
    }
    maybeConsumeEOLs();
    return "";
  }

};

}}}
