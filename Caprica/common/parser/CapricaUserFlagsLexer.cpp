#include <common/parser/CapricaUserFlagsLexer.h>

#include <cctype>
#include <map>
#include <unordered_map>

#include <common/CapricaConfig.h>
#include <common/CaselessStringComparer.h>

namespace caprica { namespace parser {

static const std::unordered_map<TokenType, const std::string> prettyTokenTypeNameMap{
  { TokenType::Unknown, "Unknown" },
  { TokenType::END, "EOF" },
  { TokenType::Identifier, "Identifier" },
  { TokenType::Integer, "Integer" },
  { TokenType::LCurly, "{" },
  { TokenType::RCurly, "}" },

  { TokenType::kFlag, "Flag" },
  { TokenType::kFunction, "Function" },
  { TokenType::kProperty, "Property" },
  { TokenType::kPropertyGroup, "PropertyGroup" },
  { TokenType::kScript, "Script" },
  { TokenType::kStructMember, "StructMember" },
  { TokenType::kVariable, "Variable" },
};

const std::string CapricaUserFlagsLexer::Token::prettyTokenType(TokenType tp) {
  auto f = prettyTokenTypeNameMap.find(tp);
  if (f == prettyTokenTypeNameMap.end())
    CapricaError::logicalFatal("Unable to determine the pretty form of token type %i!", (int32_t)tp);
  return f->second;
}

void CapricaUserFlagsLexer::setTok(TokenType tp, const CapricaFileLocation& loc) {
  cur = Token(tp, loc);
}

void CapricaUserFlagsLexer::setTok(Token& tok) {
  cur = tok;
}

static const std::map<std::string, TokenType, CaselessStringComparer> keywordMap {
  { "flag", TokenType::kFlag },
  { "function", TokenType::kFunction },
  { "property", TokenType::kProperty },
  { "propertygroup", TokenType::kPropertyGroup },
  { "script", TokenType::kScript },
  { "structmember", TokenType::kStructMember },
  { "variable", TokenType::kVariable },
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

      while (isdigit(peekChar()))
        str.put(getChar());

      auto i = std::stoul(str.str());
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
    case 'Z':
    {
      std::ostringstream str;
      str.put(c);

      while (isalpha(peekChar()))
        str.put(getChar());

      auto ident = str.str();
      auto f = keywordMap.find(ident);
      if (f != keywordMap.end())
        return setTok(f->second, baseLoc);

      auto tok = Token(TokenType::Identifier, baseLoc);
      tok.sValue = ident;
      return setTok(tok);
    }

    case '/':
    {
      if (peekChar() == '*') {
        // Multiline comment.
        getChar();

        while (peekChar() != -1) {
          if (peekChar() == '\r' || peekChar() == '\n') {
            auto c2 = getChar();
            if (c2 == '\r' && peekChar() == '\n')
              getChar();
            location.nextLine();
          }

          if (getChar() == '*' && peekChar() == '/') {
            getChar();
            goto StartOver;
          }
        }

        CapricaError::fatal(location, "Unexpected EOF before the end of a multiline comment!");
      } else if (peekChar() != '/') {
        CapricaError::fatal(location, "Unexpected character '/'!");
      }

      // Single line comment.
      while (peekChar() != '\r' && peekChar() != '\n' && peekChar() != -1)
        getChar();
      goto StartOver;
    }

    case '\r':
    case '\n':
    {
      if (c == '\r' && peekChar() == '\n')
        getChar();
      location.nextLine();
      goto StartOver;
    }

    case ' ':
    case '\t':
    {
      while (peekChar() == ' ' || peekChar() == '\t')
        getChar();
      goto StartOver;
    }

    default:
      CapricaError::fatal(baseLoc, "Unexpected character '%c'!", (char)c);
  }
}

}}
