#pragma once

#include <cstring>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>

#include <CapricaError.h>

#include <papyrus/parser/PapyrusFileLocation.h>

namespace caprica { namespace pex { namespace parser {

typedef papyrus::parser::PapyrusFileLocation PapyrusFileLocation;

enum class TokenType : int32_t
{
  Unknown,

  EOL,
  // This is EOF, but EOF is a stdlib define :(
  END,

  Identifier,
  String,
  Integer,
  Float,

  Dot,
  LineNumer,
};

struct PexAsmLexer
{
  struct Token final
  {
    TokenType type{ TokenType::Unknown };
    PapyrusFileLocation location;
    std::string sValue{ "" };
    int64_t iValue{ };
    float fValue{ };

    Token(TokenType tp, PapyrusFileLocation loc) : type(tp), location(loc) { }

    // When fixing this, fix expect() to output expected token type as well.
    std::string prettyString() const {
      switch (type) {
        case TokenType::Identifier:
          return "Identifier(" + sValue + ")";
        case TokenType::String:
          return "String(\"" + sValue + "\")";
        case TokenType::Integer:
        {
          std::ostringstream str;
          str << "Integer(" << iValue << ")";
          return str.str();
        }
        case TokenType::Float:
        {
          std::ostringstream str;
          str << "Float(" << fValue << ")";
          return str.str();
        }
        default:
          return prettyTokenType(type);
      }
    }

    static const std::string prettyTokenType(TokenType tp);
  };

  PexAsmLexer(std::string file)
    : filename(file),
      strm(file, std::ifstream::binary),
      location(file, 1, 0),
      cur(TokenType::Unknown, PapyrusFileLocation{ "", 0, 0 })
  {
    consume(); // set the first token.
  }

  ~PexAsmLexer() = default;

protected:
  std::string filename;
  Token cur;

  void consume();

private:
  std::ifstream strm;
  PapyrusFileLocation location;

  int getChar() {
    location.column++;
    return strm.get();
  }
  int peekChar() {
    return strm.peek();
  }
  void setTok(TokenType tp, int consumeChars = 0);
  void setTok(Token& tok);
};

}}}
