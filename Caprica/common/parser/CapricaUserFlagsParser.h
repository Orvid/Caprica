#pragma once

#include <string>

#include <common/CapricaUserFlagsDefinition.h>
#include <common/parser/CapricaUserFlagsLexer.h>

namespace caprica { namespace parser {

struct CapricaUserFlagsParser final : private CapricaUserFlagsLexer
{
  explicit CapricaUserFlagsParser(std::string file) : CapricaUserFlagsLexer(file) { }
  ~CapricaUserFlagsParser() = default;

  void parseUserFlags(CapricaUserFlagsDefinition& def);
  
private:
  void expect(TokenType tp) {
    if (cur.type != tp)
      CapricaError::fatal(cur.location, "Expected '" + Token::prettyTokenType(tp) + "' got '" + cur.prettyString() + "'!");
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
};

}}
