#pragma once

#include <cstring>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>

#include <common/CapricaError.h>
#include <common/CapricaFileLocation.h>

namespace caprica { namespace papyrus { namespace parser {

enum class TokenType : int32_t
{
  Unknown,

  EOL,
  // This is EOF, but EOF is a stdlib define :(
  END,

  Identifier,
  DocComment,
  String,
  Integer,
  Float,

  LParen,
  RParen,
  LSquare,
  RSquare,
  Dot,
  Comma,

  Equal,
  Exclaim,

  Plus,
  PlusEqual,
  Minus,
  MinusEqual,
  Mul,
  MulEqual,
  Div,
  DivEqual,
  Mod,
  ModEqual,

  CmpEq,
  CmpNeq,
  CmpLt,
  CmpLte,
  CmpGt,
  CmpGte,

  BooleanOr,
  BooleanAnd,

  // Unfortunately, as there are also literal floats and strings,
  // we need to prefix the keyword tokens.
  kAs,
  kAuto,
  kAutoReadOnly,
  kBool,
  kElse,
  kElseIf,
  kEndEvent,
  kEndFunction,
  kEndIf,
  kEndProperty,
  kEndState,
  kEndWhile,
  kEvent,
  kExtends,
  kFalse,
  kFloat,
  kFunction,
  kGlobal,
  kIf,
  kImport,
  kInt,
  kLength,
  kNative,
  kNew,
  kNone,
  kParent,
  kProperty,
  kReturn,
  kScriptName,
  kSelf,
  kState,
  kString,
  kTrue,
  kWhile,

  // Additional speculative keywords for FO4
  kConst,
  kEndPropertyGroup,
  kEndStruct,
  kIs,
  kPropertyGroup,
  kStruct,
  kVar,

  // Language extension keywords
  kBreak,
  kCase,
  kContinue,
  kDefault,
  kDo,
  kEndFor,
  kEndForEach,
  kEndSwitch,
  kFor,
  kForEach,
  kIn,
  kLoopWhile,
  kSwitch,
  kTo,
};

struct PapyrusLexer
{
  struct Token final
  {
    TokenType type{ TokenType::Unknown };
    CapricaFileLocation location;
    std::string sValue{ "" };
    int32_t iValue{ };
    float fValue{ };

    Token(TokenType tp, CapricaFileLocation loc) : type(tp), location(loc) { }

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

  PapyrusLexer(std::string file) 
    : filename(file),
      strm(file, std::ifstream::binary),
      location(file, 1, 0),
      cur(TokenType::Unknown, CapricaFileLocation{ "", 0, 0 })
  {
    consume(); // set the first token.
  }

  ~PapyrusLexer() = default;

protected:
  std::string filename;
  Token cur;

  void consume();
  CapricaFileLocation consumeLocation() {
    auto loc = cur.location;
    consume();
    return loc;
  }
  // Use this sparingly, as it means
  // tokens get lexed multiple times.
  Token peekToken(int distance = 0);

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
  void setTok(TokenType tp, const CapricaFileLocation& loc, int consumeChars = 0);
  void setTok(Token& tok);
};

}}}
