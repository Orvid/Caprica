#pragma once

#include <istream>

#include <papyrus/PapyrusScript.h>
#include <papyrus/parser/PapyrusLexer.h>

namespace caprica { namespace papyrus { namespace parser {

struct PapyrusParser : private PapyrusLexer
{
  PapyrusParser(std::istream& strm) : PapyrusLexer(strm) { }
  ~PapyrusParser() = default;

  PapyrusScript* parseScript();
  
private:
  PapyrusObject* parseObject(PapyrusScript* script);
  PapyrusState* parseState(PapyrusScript* script, PapyrusObject* object, bool isAuto);
  PapyrusFunction* parseFunction(PapyrusScript* script, PapyrusObject* object, PapyrusState* state, PapyrusType returnType, TokenType endToken);
  PapyrusProperty* parseProperty(PapyrusScript* script, PapyrusObject* object, bool isConst, PapyrusType type);
  PapyrusVariable* parseVariable(PapyrusScript* script, PapyrusObject* object, bool isConst, PapyrusType type);
  PapyrusUserFlags parseUserFlags(PapyrusUserFlags validFlags);

  PapyrusType expectConsumePapyrusType();
  PapyrusValue expectConsumePapyrusValue();

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
      maybeConsumeEOLs();
      return str;
    }
    maybeConsumeEOLs();
    return "";
  }

};

}}}
