#pragma once

#include <cstring>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>

#include <common/CapricaError.h>
#include <common/CapricaFileLocation.h>
#include <common/FSUtils.h>

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
  kBetaOnly,
  kBool,
  kConst,
  kCustomEvent,
  kDebugOnly,
  kElse,
  kElseIf,
  kEndEvent,
  kEndFunction,
  kEndGroup,
  kEndIf,
  kEndProperty,
  kEndState,
  kEndStruct,
  kEndWhile,
  kEvent,
  kExtends,
  kFalse,
  kFloat,
  kFunction,
  kGlobal,
  kGroup,
  kIf,
  kImport,
  kInt,
  kIs,
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
  kStruct,
  kTrue,
  kVar,
  kWhile,

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
  kStep,
  kSwitch,
  kTo,
};

struct PapyrusLexer
{
  struct Token final
  {
    TokenType type{ TokenType::Unknown };
    CapricaFileLocation location;
    std::string sValue{ };
    int32_t iValue{ };
    float fValue{ };

    explicit Token(TokenType tp, const CapricaFileLocation& loc) : type(tp), location(loc) { }
    // This is deleted deliberately. The assign operator of the std::string in the location
    // is not cheap, but that string value is never going to be different within the same file,
    // so there's no need to keep calling it. Instead we modify everything else about the
    // token and location.
    Token(const Token&) = delete;
    ~Token() = default;

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

  explicit PapyrusLexer(const std::string& file)
    : filename(file),
      location(file, 1, 0),
      cur(TokenType::Unknown, CapricaFileLocation{ file, 0, 0 })
  {
    auto s = FSUtils::Cache::cachedReadFull(file);
    strm = _strdup(s.c_str());
    strmOriginal = strm;
    strmLen = s.size();
    consume(); // set the first token.
  }
  PapyrusLexer(const PapyrusLexer&) = delete;
  ~PapyrusLexer() {
    if (strmOriginal)
      free((void*)strmOriginal);
  }

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
  TokenType peekTokenType(int distance = 0);

private:
  const char* strmOriginal{ nullptr };
  const char* strm{ nullptr };
  size_t strmI{ 0 };
  size_t strmLen{ 0 };
  CapricaFileLocation location;

  int getChar() {
    if (strmI >= strmLen)
      return -1;
    location.column++;
    strmI++;
    auto c = *strm;
    strm++;
    return (unsigned)c;
  }
  int peekChar() {
    if (strmI + 1 >= strmLen)
      return -1;
    return *strm;
  }
  void setTok(TokenType tp, const CapricaFileLocation::Partial& loc, int consumeChars = 0);
};

}}}
