#include <pex/parser/PexAsmLexer.h>

#include <cctype>
#include <map>

#include <CapricaConfig.h>

namespace caprica { namespace pex { namespace parser {

static const std::map<TokenType, const std::string> prettyTokenTypeNameMap{
  { TokenType::Unknown, "Unknown" },
  { TokenType::EOL, "EOL" },
  { TokenType::END, "EOF" },
  { TokenType::Identifier, "Identifier" },
  { TokenType::String, "String" },
  { TokenType::Integer, "Integer" },
  { TokenType::Float, "Float" },
  { TokenType::Dot, "." },
  { TokenType::LineNumer, ";@line" },
};

const std::string PexAsmLexer::Token::prettyTokenType(TokenType tp) {
  auto f = prettyTokenTypeNameMap.find(tp);
  if (f == prettyTokenTypeNameMap.end())
    CapricaError::logicalFatal("Unable to determine the pretty form of token type %i!", (int32_t)tp);
  return f->second;
}

void PexAsmLexer::setTok(TokenType tp, int consumeChars) {
  cur = Token(tp, location);
  for (int i = 0; i < consumeChars; i++)
    getChar();
}

void PexAsmLexer::setTok(Token& tok) {
  cur = tok;
  cur.location = location;
}

void PexAsmLexer::consume() {
StartOver:
  auto c = getChar();
  
  switch (c) {
    case -1:
      return setTok(TokenType::END);
    case '.':
      return setTok(TokenType::Dot);

    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    {
      std::ostringstream str;
      str.put(c);

      // It's hex.
      if (c == '0' && peekChar() == 'x') {
        str.put(getChar());
        while (isxdigit(peekChar()))
          str.put(getChar());
        
        auto i = std::stoul(str.str(), nullptr, 16);
        auto tok = Token(TokenType::Integer, location);
        tok.iValue = (int32_t)i;
        return setTok(tok);
      }

      // Either normal int or float.
      while (isdigit(peekChar()))
        str.put(getChar());

      // It's a float.
      if (peekChar() == '.') {
        str.put(getChar());
        while (isdigit(peekChar()))
          str.put(getChar());

        // Allow e+ notation.
        if (peekChar() == 'e') {
          str.put(getChar());
          if (getChar() != '+')
            CapricaError::fatal(location, "Unexpected character 'e'!");
          str.put('+');

          while (isdigit(peekChar()))
            str.put(getChar());
        }

        auto f = std::stof(str.str());
        auto tok = Token(TokenType::Float, location);
        tok.fValue = f;
        return setTok(tok);
      }

      auto i = std::stoull(str.str());
      auto tok = Token(TokenType::Integer, location);
      tok.iValue = (int64_t)i;
      return setTok(tok);
    }

    case ':':
    case '_':
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
    case 'Z':
    {
      std::ostringstream str;
      str.put(c);

      // We allow the characters for types in this as well.
      while (isalnum(peekChar()) || peekChar() == '_' || peekChar() == ':' || peekChar() == '#' || peekChar() == '[' || peekChar() == ']')
        str.put(getChar());

      auto ident = str.str();
      auto tok = Token(TokenType::Identifier, location);
      tok.sValue = ident;
      return setTok(tok);
    }

    case '"':
    {
      std::ostringstream str;

      while (peekChar() != '"' && peekChar() != '\r' && peekChar() != '\n' && peekChar() != -1) {
        if (peekChar() == '\\') {
          getChar();
          auto escapeChar = getChar();
          switch (escapeChar) {
            case 'n':
              str.put('\n');
              break;
            case 't':
              str.put('\t');
              break;
            case '\\':
              str.put('\\');
              break;
            case '"':
              str.put('"');
              break;
            case -1:
              CapricaError::fatal(location, "Unexpected EOF before the end of the string.");
            default:
              CapricaError::fatal(location, "Unrecognized escape sequence: '\\%c'", (char)escapeChar);
          }
        } else {
          str.put(getChar());
        }
      }

      if (peekChar() != '"')
        CapricaError::fatal(location, "Unclosed string!");
      getChar();

      auto tok = Token(TokenType::String, location);
      tok.sValue = str.str();
      return setTok(tok);
    }

    case ';':
    {
      if (getChar() != '@' || getChar() != 'l' || getChar() != 'i' || getChar() != 'n' || getChar() != 'e')
        CapricaError::fatal(location, "Unexpected character sequence that looked line a ;@line directive!");
      return setTok(TokenType::LineNumer);
    }

    case '\r':
    case '\n':
    {
      if (c == '\r' && peekChar() == '\n')
        getChar();
      location.line++;
      location.column = 0;
      return setTok(TokenType::EOL);
    }

    case ' ':
    case '\t':
    {
      while (peekChar() == ' ' || peekChar() == '\t')
        getChar();
      goto StartOver;
    }

    default:
      CapricaError::fatal(location, "Unexpected character '%c'!", (char)c);
  }
}

}}}
