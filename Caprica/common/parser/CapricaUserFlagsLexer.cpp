#include <common/parser/CapricaUserFlagsLexer.h>

#include <cctype>
#include <map>
#include <unordered_map>
#include <utility>

#include <common/FakeScripts.h>
#include <common/CapricaConfig.h>
#include <common/CaselessStringComparer.h>

namespace caprica { namespace parser {

static const std::unordered_map<TokenType, const std::string> prettyTokenTypeNameMap {
  {TokenType::Unknown,     "Unknown"   },
  { TokenType::END,        "EOF"       },
  { TokenType::Identifier, "Identifier"},
  { TokenType::Integer,    "Integer"   },
  { TokenType::LCurly,     "{"         },
  { TokenType::RCurly,     "}"         },
  { TokenType::And,        "&"         },

  { TokenType::kFlag,      "Flag"      },
  { TokenType::kFunction,  "Function"  },
  { TokenType::kGroup,     "Group"     },
  { TokenType::kProperty,  "Property"  },
  { TokenType::kScript,    "Script"    },
  { TokenType::kStructVar, "StructVar" },
  { TokenType::kVariable,  "Variable"  },
};

const std::string CapricaUserFlagsLexer::Token::prettyTokenType(TokenType tp) {
  auto f = prettyTokenTypeNameMap.find(tp);
  if (f == prettyTokenTypeNameMap.end())
    CapricaReportingContext::logicalFatal("Unable to determine the pretty form of token type {}!", std::to_underlying(tp));
  return f->second;
}

void CapricaUserFlagsLexer::setTok(TokenType tp, CapricaFileLocation loc) {
  cur = Token(tp, loc);
  cur.location.endOffset = location.startOffset;
}

void CapricaUserFlagsLexer::setTok(Token& tok) {
  cur = tok;
}

static const caseless_unordered_identifier_map<TokenType> keywordMap {
  {"flag",       TokenType::kFlag     },
  { "function",  TokenType::kFunction },
  { "group",     TokenType::kGroup    },
  { "property",  TokenType::kProperty },
  { "script",    TokenType::kScript   },
  { "structvar", TokenType::kStructVar},
  { "variable",  TokenType::kVariable },
};

void CapricaUserFlagsLexer::consume() {
StartOver:
  auto baseLoc = location;
  auto c = getChar();

  switch (c) {
    case -1:
      return setTok(TokenType::END, baseLoc);
    case '{':
      return setTok(TokenType::LCurly, baseLoc);
    case '}':
      return setTok(TokenType::RCurly, baseLoc);
    case '&':
      return setTok(TokenType::And, baseLoc);

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      std::string str;
      str.append(1, char(c));

      while (isdigit(peekChar()))
        str.append(1, char(getChar()));

      auto i = std::stoul(str);
      auto tok = Token(TokenType::Integer, baseLoc);
      tok.iValue = (int32_t)i;
      return setTok(tok);
    }

    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z': {
      std::string str;
      str.append(1, char(c));

      while (isalpha(peekChar()))
        str.append(1, char(getChar()));

      auto f = keywordMap.find(str);
      if (f != keywordMap.end() && (f->second != TokenType::kStructVar || conf::Papyrus::game > GameID::Skyrim))
        return setTok(f->second, baseLoc);

      auto tok = Token(TokenType::Identifier, baseLoc);
      tok.sValue = std::move(str);
      return setTok(tok);
    }

    case '/': {
      if (peekChar() == '*') {
        // Multiline comment.
        getChar();

        while (peekChar() != -1) {
          if (peekChar() == '\r' || peekChar() == '\n') {
            auto c2 = getChar();
            if (c2 == '\r' && peekChar() == '\n')
              getChar();
            reportingContext.pushNextLineOffset(location);
          }

          if (getChar() == '*' && peekChar() == '/') {
            getChar();
            goto StartOver;
          }
        }

        reportingContext.error(location, "Unexpected EOF before the end of a multiline comment!");
      } else if (peekChar() != '/') {
        reportingContext.error(location, "Unexpected character '/'!");
      }

      // Single line comment.
      while (peekChar() != '\r' && peekChar() != '\n' && peekChar() != -1)
        getChar();
      goto StartOver;
    }

    case '\r':
    case '\n': {
      if (c == '\r' && peekChar() == '\n')
        getChar();
      reportingContext.pushNextLineOffset(location);
      goto StartOver;
    }

    case ' ':
    case '\t': {
      while (peekChar() == ' ' || peekChar() == '\t')
        getChar();
      goto StartOver;
    }

    default:
      reportingContext.error(baseLoc, "Unexpected character '{}'!", (char)c);
      goto StartOver;
  }
}
CapricaUserFlagsLexer::CapricaUserFlagsLexer(CapricaReportingContext& repCtx, const std::string& file)
    : filename(file), cur(TokenType::Unknown), reportingContext(repCtx) {
  if (filename.starts_with("fake://")){
    strmString.str(FakeScripts::getFakeFlagsFile(conf::Papyrus::game).to_string());
    strm = &strmString;
  } else {
    strmFile.open(filename, std::ifstream::binary);
    if (!strmFile.is_open()) {
      CapricaReportingContext::logicalFatal("Unable to open file '%s'!", filename.c_str());
    }
    strm = &strmFile;
  }
  consume(); // set the first token.
}

}}
