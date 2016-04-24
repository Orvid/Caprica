#pragma once

#include <cstdint>
#include <numeric>
#include <string>

#include <common/CapricaReportingContext.h>

#include <pex/PexFile.h>
#include <pex/parser/PexAsmLexer.h>

namespace caprica { namespace pex { namespace parser {

struct PexAsmParser final : private PexAsmLexer
{
  explicit PexAsmParser(CapricaReportingContext& repCtx, const std::string& file) : PexAsmLexer(repCtx, file) { }
  PexAsmParser(const PexAsmParser&) = delete;
  ~PexAsmParser() = default;

  PexFile* parseFile();

private:
  PexFunction* parseFunction(PexFile* file, PexDebugFunctionInfo* debInfo, std::string& funcNameOut);

  void expect(TokenType tp) {
    if (cur.type != tp) {
      if (tp == TokenType::EOL && cur.type == TokenType::END)
        return;
      reportingContext.fatal(cur.location, "Expected '" + Token::prettyTokenType(tp) + "' got '" + cur.prettyString() + "'!");
    }
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

  void maybeExpectConsumeConstFlag(bool* isConst) {
    assert(isConst != nullptr);
    if (cur.type != TokenType::EOL) {
      expect(TokenType::Identifier);
      if (!_stricmp(cur.sValue.c_str(), "const")) {
        *isConst = true;
        consume();
      }
    }
  }

  bool maybeConsumeTokEOL(TokenType tp) {
    if (maybeConsume(tp)) {
      expectConsumeEOL();
      return true;
    }
    return false;
  }

  void expectConsumeEOL() {
    expectConsume(TokenType::EOL);
  }

  std::string expectConsumeIdent() {
    expect(TokenType::Identifier);
    auto val = cur.sValue;
    consume();
    return val;
  }

  std::string expectConsumeIdentEOL() {
    auto val = expectConsumeIdent();
    expectConsumeEOL();
    return val;
  }

  PexString expectConsumePexIdent(PexFile* file) {
    return file->getString(expectConsumeIdent());
  }

  PexString expectConsumePexIdentEOL(PexFile* file) {
    return file->getString(expectConsumeIdentEOL());
  }

  PexString maybeConsumePexIdentEOL(PexFile* file) {
    PexString val;
    if (cur.type == TokenType::Identifier)
      val = file->getString(expectConsumeIdent());
    else
      val = file->getString("");
    expectConsumeEOL();
    return val;
  }

  std::string expectConsumeString() {
    expect(TokenType::String);
    auto val = cur.sValue;
    consume();
    return val;
  }

  std::string expectConsumeStringEOL() {
    auto val = expectConsumeString();
    expectConsumeEOL();
    return val;
  }

  PexString expectConsumePexString(PexFile* file) {
    return file->getString(expectConsumeString());
  }

  PexString expectConsumePexStringEOL(PexFile* file) {
    return file->getString(expectConsumeStringEOL());
  }

  int32_t expectConsumeInteger() {
    expect(TokenType::Integer);
    auto val = cur.iValue;
    if (val >= std::numeric_limits<int32_t>::max() || val <= std::numeric_limits<int32_t>::min())
      reportingContext.fatal(cur.location, "Integer value '%ll' outside of the range of 32-bits!", val);
    consume();
    return (int32_t)val;
  }

  int32_t expectConsumeIntegerEOL() {
    auto val = expectConsumeInteger();
    expectConsumeEOL();
    return val;
  }

  int64_t expectConsumeLongIntegerEOL() {
    expect(TokenType::Integer);
    auto val = cur.iValue;
    consume();
    expectConsumeEOL();
    return val;
  }

  PexUserFlags expectConsumeUserFlags() {
    auto v = expectConsumeIntegerEOL();
    PexUserFlags flags;
    flags.data = v;
    return flags;
  }

  PexValue expectConsumeValue(PexFile* file);
  PexValue expectConsumeValueEOL(PexFile* file) {
    auto val = expectConsumeValue(file);
    expectConsumeEOL();
    return val;
  }
};

}}}
