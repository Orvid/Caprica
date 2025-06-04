#pragma once

#include <cstring>
#include <functional>
#include <string>
#include <string_view>

#include <common/allocators/ChainedPool.h>
#include <common/CapricaFileLocation.h>
#include <common/CapricaReportingContext.h>
#include <common/CapricaStats.h>
#include <common/FSUtils.h>
#include <common/GameID.h>
#include <common/identifier_ref.h>
#include <common/UtilMacros.h>

namespace caprica { namespace papyrus { namespace parser {

enum class TokenType : int32_t {
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
  kTrue,
  kWhile,

  // Fallout4 / Fallout76 keywords
  kBetaOnly,
  kConst,
  kCustomEvent,
  kCustomEventName,
  kDebugOnly,
  kEndGroup,
  kEndStruct,
  kGroup,
  kScriptEventName,
  kStruct,
  kVar,

  // Starfield keywords
  kGuard,
  kEndGuard,
  kTryGuard,

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
constexpr TokenType SKYRIM_MAX_KEYWORD = TokenType::kWhile;
constexpr TokenType FALLOUT4_MAX_KEYWORD = TokenType::kVar;
constexpr TokenType FALLOUT76_MAX_KEYWORD = FALLOUT4_MAX_KEYWORD;
constexpr TokenType STARFIELD_MAX_KEYWORD = TokenType::kTryGuard;

constexpr bool keywordIsLanguageExtension(TokenType tp) {
  return tp >= TokenType::kBreak && tp <= TokenType::kTo;
}
const bool keywordCanBeIdentifier(TokenType tp);
constexpr bool keywordIsInGame(TokenType tp, GameID game, bool includeExtensions = false) {
  if (includeExtensions && keywordIsLanguageExtension(tp))
    return true;

  switch (game) {
    case GameID::Skyrim:
      return tp <= SKYRIM_MAX_KEYWORD;
    case GameID::Fallout4:
      return tp <= FALLOUT4_MAX_KEYWORD;
    case GameID::Fallout76:
      return tp <= FALLOUT76_MAX_KEYWORD;
    case GameID::Starfield:
      return tp <= STARFIELD_MAX_KEYWORD;
    default:
      return false;
  }
}

struct PapyrusLexer {
  struct Token final {
    TokenType type { TokenType::Unknown };
    union InnerValue {
      identifier_ref s;
      int32_t i;
      float f;

      InnerValue() : s() { }
    } val;
    CapricaFileLocation location {};

    explicit Token(TokenType tp) : type(tp) { }
    Token(const Token&) = delete;
    Token(Token&&) = default;
    Token& operator=(const Token&) = delete;
    Token& operator=(Token&&) = default;
    ~Token() = default;

    std::string prettyString() const {
      switch (type) {
        case TokenType::Identifier:
          return "Identifier(" + val.s.to_string() + ")";
        case TokenType::String:
          return "String(\"" + val.s.to_string() + "\")";
        case TokenType::Integer:
          return "Integer(" + std::to_string(val.i) + ")";
        case TokenType::Float:
          return "Float(" + std::to_string(val.f) + ")";
        default:
          return std::string(prettyTokenType(type));
      }
    }

    static std::string_view prettyTokenType(TokenType tp);
  };

  explicit PapyrusLexer(CapricaReportingContext& repCtx, const std::string& file, std::string_view data)
      : filename(file), reportingContext(repCtx), alloc(new allocators::ChainedPool(1024 * 4)) {
    CapricaStats::lexedFilesCount++;
    strm = data.data();
    strmLen = data.size();
    consume(); // set the first token.
  }
  PapyrusLexer(const PapyrusLexer&) = delete;
  ~PapyrusLexer() = default;

protected:
  allocators::ChainedPool* alloc;
  CapricaReportingContext& reportingContext;
  std::string filename;
  Token cur { TokenType::Unknown };

  void consume();
  ALWAYS_INLINE
  CapricaFileLocation consumeLocation() {
    auto loc = cur.location;
    consume();
    return loc;
  }
  // Max distance is 2, and you must
  // not attempt to peek past those
  // 3 tokens until all 3 have been
  // consumed.
  TokenType peekTokenType(int distance = 0);

private:
  const char* strm { nullptr };
  size_t strmI { 0 };
  size_t strmLen { 0 };
  CapricaFileLocation location {};
  static constexpr size_t MaxPeekedTokens = 3;
  int peekedTokenI { 0 };
  int peekedTokenCount { 0 };
  Token peekedTokens[MaxPeekedTokens] {
    Token { TokenType::Unknown },
    Token { TokenType::Unknown },
    Token { TokenType::Unknown },
  };

  ALWAYS_INLINE
  int getChar() {
    if (strmI >= strmLen)
      return -1;
    location.startOffset++;
    strmI++;
    auto c = *strm;
    strm++;
    return (unsigned)c;
  }

  ALWAYS_INLINE
  void advanceChars(int distance) {
    location.startOffset += distance;
    strmI += distance;
    strm += distance;
  }

  ALWAYS_INLINE
  int peekChar() {
    if (strmI + 1 > strmLen)
      return -1;
    return *strm;
  }

  NEVER_INLINE
  void realConsume();

  ALWAYS_INLINE
  void setTok(TokenType tp, CapricaFileLocation loc, int consumeChars = 0);
};

}}}
