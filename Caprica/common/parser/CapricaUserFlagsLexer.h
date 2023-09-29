#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include <common/CapricaFileLocation.h>
#include <common/CapricaReportingContext.h>

namespace caprica { namespace parser {

enum class TokenType : int32_t {
  Unknown,

  // This is EOF, but EOF is a stdlib define :(
  END,

  Identifier,
  Integer,
  LCurly,
  RCurly,
  And,

  kFlag,
  kFunction,
  kGroup,
  kProperty,
  kScript,
  kStructVar,
  kVariable,
};

struct CapricaUserFlagsLexer {
  struct Token final {
    TokenType type { TokenType::Unknown };
    CapricaFileLocation location {};
    std::string sValue {};
    int32_t iValue {};

    explicit Token(TokenType tp) : type(tp) { }
    explicit Token(TokenType tp, CapricaFileLocation loc) : type(tp), location(loc) { }
    Token(const Token&) = default;
    ~Token() = default;

    // TODO: When fixing this, fix expect() to output expected token type as well.
    std::string prettyString() const {
      switch (type) {
        case TokenType::Identifier:
          return "Identifier(" + sValue + ")";
        case TokenType::Integer: {
          std::ostringstream str;
          str << "Integer(" << iValue << ")";
          return str.str();
        }
        default:
          return prettyTokenType(type);
      }
    }

    static const std::string prettyTokenType(TokenType tp);
  };

  explicit CapricaUserFlagsLexer(CapricaReportingContext& repCtx, const std::string& file);
  CapricaUserFlagsLexer(const CapricaUserFlagsLexer&) = delete;
  ~CapricaUserFlagsLexer() = default;

protected:
  CapricaReportingContext& reportingContext;
  std::string filename;
  Token cur;

  void consume();

private:
  std::istream * strm;
  std::istringstream strmString;
  std::ifstream strmFile;
  CapricaFileLocation location {};

  int getChar() {
    location.startOffset++;
    return strm->get();
  }
  int peekChar() { return strm->peek(); }
  void setTok(TokenType tp, CapricaFileLocation loc);
  void setTok(Token& tok);
};

}}
