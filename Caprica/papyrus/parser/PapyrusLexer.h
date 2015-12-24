#pragma once

#include <cstring>
#include <istream>
#include <functional>
#include <sstream>
#include <string>

namespace caprica { namespace papyrus { namespace parser {

struct CaselessStringComparer : public std::binary_function<std::string, std::string, bool>
{
  bool operator()(const std::string &lhs, const std::string &rhs) const {
    return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
  }
};

enum class TokenType
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
  RSqaure,
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

  LogicalOr,
  LogicalAnd,

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
  kPropertyGroup,
  kStruct,
  kVar,
};

struct PapyrusLexer
{
  struct Token final
  {
    TokenType type{ TokenType::Unknown };
    size_t line{ };
    std::string sValue{ "" };
    int32_t iValue{ };
    float fValue{ };

    Token(TokenType tp) : type(tp) { }

    std::string asString() const {
      std::ostringstream str;
      str << (int)type << ": " << line << " s(" << sValue << ") i(" << iValue << ") f(" << fValue << ")";
      return str.str();
    }

    // When fixing this, fix expect() to output expected token type as well.
    std::string prettyString() const {
      return asString();
    }
  };

  PapyrusLexer(std::istream& input) : strm(input) {
    consume(); // set the first token.
  }
  ~PapyrusLexer() = default;

protected:
  Token cur{ TokenType::Unknown };

  void consume();

  [[noreturn]]
  void fatalError(const std::string& msg) {
    // TODO: Expand on this, making sure to write things like the
    // line number to stderr before dying.
    throw new std::runtime_error(msg);
  }

private:
  std::istream& strm;
  size_t lineNum{ 1 };

  void setTok(TokenType tp, int consumeChars = 0);
  void setTok(Token& tok);
};

}}}
