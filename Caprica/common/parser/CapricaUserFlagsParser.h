#pragma once

#include <string>

#include <common/CapricaUserFlagsDefinition.h>
#include <common/parser/CapricaUserFlagsLexer.h>

namespace caprica { namespace parser {

struct CapricaUserFlagsParser final : private CapricaUserFlagsLexer
{
  explicit CapricaUserFlagsParser(CapricaReportingContext& repCtx, const std::string& file) : CapricaUserFlagsLexer(repCtx, file) { }
  CapricaUserFlagsParser(const CapricaUserFlagsParser&) = delete;
  ~CapricaUserFlagsParser() = default;

  void parseUserFlags(CapricaUserFlagsDefinition& def);
  
private:
  void expect(TokenType tp) {
    if (cur.type != tp)
      reportingContext.fatal(cur.location, "Expected '" + Token::prettyTokenType(tp) + "' got '" + cur.prettyString() + "'!");
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
