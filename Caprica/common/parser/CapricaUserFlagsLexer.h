#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include <common/CapricaError.h>
#include <common/CapricaFileLocation.h>

namespace caprica { namespace parser {

enum class TokenType : int32_t
{
  Unknown,

  // This is EOF, but EOF is a stdlib define :(
  END,

  Identifier,
  Integer,
  LCurly,
  RCurly,

  kFlag,
  kFunction,
  kProperty,
  kPropertyGroup,
  kScript,
  kStructMember,
  kVariable,
};

struct CapricaUserFlagsLexer
{
  struct Token final
  {
    TokenType type{ TokenType::Unknown };
    CapricaFileLocation location;
    std::string sValue{ };
    int32_t iValue{ };

    explicit Token(TokenType tp, const CapricaFileLocation& loc) : type(tp), location(loc) { }
    Token(const Token&) = default;
    ~Token() = default;

    // When fixing this, fix expect() to output expected token type as well.
    std::string prettyString() const {
      switch (type) {
        case TokenType::Identifier:
          return "Identifier(" + sValue + ")";
        case TokenType::Integer:
        {
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

  explicit CapricaUserFlagsLexer(std::string file)
    : filename(file),
      strm(file, std::ifstream::binary),
      location(file, 1, 0),
      cur(TokenType::Unknown, CapricaFileLocation{ "", 0, 0 })
  {
    consume(); // set the first token.
  }
  CapricaUserFlagsLexer(const CapricaUserFlagsLexer&) = delete;
  ~CapricaUserFlagsLexer() = default;

protected:
  std::string filename;
  Token cur;

  void consume();

private:
  std::ifstream strm;
  CapricaFileLocation location;

  int getChar() {
    location.column++;
    return strm.get();
  }
  int peekChar() {
    return strm.peek();
  }
  void setTok(TokenType tp, const CapricaFileLocation& loc);
  void setTok(Token& tok);
};

}}
