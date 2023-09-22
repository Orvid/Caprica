#pragma once

#include <cstring>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>

#include <common/CapricaFileLocation.h>
#include <common/CapricaReportingContext.h>

namespace caprica { namespace pex { namespace parser {

enum class TokenType : int32_t {
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
  kEndGuardTable,
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
  kGuard,
  kGuardTable,
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

struct PexAsmLexer {
  struct Token final {
    TokenType type { TokenType::Unknown };
    CapricaFileLocation location {};
    std::string sValue { "" };
    int64_t iValue {};
    float fValue {};

    explicit Token(TokenType tp) : type(tp) { }
    explicit Token(TokenType tp, CapricaFileLocation loc) : type(tp), location(loc) { }

    // TODO: When fixing this, fix expect() to output expected token type as well.
    std::string prettyString() const {
      switch (type) {
        case TokenType::Identifier:
          return "Identifier(" + sValue + ")";
        case TokenType::String:
          return "String(\"" + sValue + "\")";
        case TokenType::Integer: {
          std::ostringstream str;
          str << "Integer(" << iValue << ")";
          return str.str();
        }
        case TokenType::Float: {
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

  explicit PexAsmLexer(CapricaReportingContext& repCtx, const std::string& file)
      : filename(file), strm(file, std::ifstream::binary), cur(TokenType::Unknown), reportingContext(repCtx) {
    consume(); // set the first token.
  }
  PexAsmLexer(const PexAsmLexer&) = delete;
  ~PexAsmLexer() = default;

protected:
  CapricaReportingContext& reportingContext;
  std::string filename;
  Token cur;

  void consume();

private:
  std::ifstream strm;
  CapricaFileLocation location {};

  int getChar() {
    location.fileOffset++;
    return strm.get();
  }
  int peekChar() { return strm.peek(); }
  void setTok(TokenType tp, CapricaFileLocation loc);
  void setTok(Token& tok);
};

}}}
