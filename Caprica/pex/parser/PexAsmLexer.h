#pragma once

#include <cstring>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>

#include <common/CapricaError.h>
#include <common/CapricaFileLocation.h>

namespace caprica { namespace pex { namespace parser {

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

  LineNumer,

  kAutoState,
  kAutoVar,
  kCode,
  kCompileTime,
  kComputer,
  kDocString,
  kEndCode,
  kEndFunction,
  kEndInfo,
  kEndLocalTable,
  kEndObject,
  kEndObjectTable,
  kEndParamTable,
  kEndProperty,
  kEndPropertyTable,
  kEndPropertyGroup,
  kEndPropertyGroupTable,
  kEndState,
  kEndStateTable,
  kEndStruct,
  kEndStructTable,
  kEndUserFlagsRef,
  kEndVariable,
  kEndVariableTable,
  kFlag,
  kFunction,
  kInfo,
  kInitialValue,
  kLocal,
  kLocalTable,
  kModifyTime,
  kObject,
  kObjectTable,
  kParam,
  kParamTable,
  kProperty,
  kPropertyTable,
  kPropertyGroup,
  kPropertyGroupTable,
  kReturn,
  kSource,
  kState,
  kStateTable,
  kStruct,
  kStructTable,
  kUser,
  kUserFlags,
  kUserFlagsRef,
  kVariable,
  kVariableTable,
};

struct PexAsmLexer
{
  struct Token final
  {
    TokenType type{ TokenType::Unknown };
    CapricaFileLocation location;
    std::string sValue{ "" };
    int64_t iValue{ };
    float fValue{ };

    explicit Token(TokenType tp, const CapricaFileLocation& loc) : type(tp), location(loc) { }

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

  explicit PexAsmLexer(std::string file)
    : filename(file),
      strm(file, std::ifstream::binary),
      location(file, 1, 0),
      cur(TokenType::Unknown, CapricaFileLocation{ "", 0, 0 })
  {
    consume(); // set the first token.
  }
  PexAsmLexer(const PexAsmLexer&) = delete;
  ~PexAsmLexer() = default;

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

}}}
