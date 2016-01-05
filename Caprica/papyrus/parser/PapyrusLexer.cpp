#include <papyrus/parser/PapyrusLexer.h>

#include <cctype>
#include <map>
#include <unordered_map>

#include <CapricaConfig.h>

#include <common/CaselessStringComparer.h>

namespace caprica { namespace papyrus { namespace parser {

static const std::unordered_map<TokenType, const std::string> prettyTokenTypeNameMap{
  { TokenType::Unknown, "Unknown" },
  { TokenType::EOL, "EOL" },
  { TokenType::END, "EOF" },
  { TokenType::Identifier, "Identifier" },
  { TokenType::DocComment, "Documentation Comment" },
  { TokenType::String, "String" },
  { TokenType::Integer, "Integer" },
  { TokenType::Float, "Float" },
  { TokenType::LParen, "(" },
  { TokenType::RParen, ")" },
  { TokenType::LSquare, "[" },
  { TokenType::RSquare, "]" },
  { TokenType::Dot, "." },
  { TokenType::Comma, "," },
  { TokenType::Equal, "=" },
  { TokenType::Exclaim, "!" },
  { TokenType::Plus, "+" },
  { TokenType::PlusEqual, "+=" },
  { TokenType::Minus, "-" },
  { TokenType::MinusEqual, "-=" },
  { TokenType::Mul, "*" },
  { TokenType::MulEqual, "*=" },
  { TokenType::Div, "/" },
  { TokenType::DivEqual, "/=" },
  { TokenType::Mod, "%" },
  { TokenType::ModEqual, "%=" },
  { TokenType::CmpEq, "==" },
  { TokenType::CmpNeq, "!=" },
  { TokenType::CmpLt, "<" },
  { TokenType::CmpLte, "<=" },
  { TokenType::CmpGt, ">" },
  { TokenType::CmpGte, ">=" },
  { TokenType::BooleanOr, "||" },
  { TokenType::BooleanAnd, "&&" },
  { TokenType::kAs, "As" },
  { TokenType::kAuto, "Auto" },
  { TokenType::kAutoReadOnly, "AutoReadOnly" },
  { TokenType::kBool, "Bool" },
  { TokenType::kElse, "Else" },
  { TokenType::kElseIf, "ElseIf" },
  { TokenType::kEndEvent, "EndEvent" },
  { TokenType::kEndFunction, "EndFunction" },
  { TokenType::kEndIf, "EndIf" },
  { TokenType::kEndProperty, "EndProperty" },
  { TokenType::kEndState, "EndState" },
  { TokenType::kEndWhile, "EndWhile" },
  { TokenType::kEvent, "Event" },
  { TokenType::kExtends, "Extends" },
  { TokenType::kFalse, "False" },
  { TokenType::kFloat, "Float" },
  { TokenType::kFunction, "Function" },
  { TokenType::kGlobal, "Global" },
  { TokenType::kIf, "If" },
  { TokenType::kImport, "Import" },
  { TokenType::kInt, "Int" },
  { TokenType::kLength, "Length" },
  { TokenType::kNative, "Native" },
  { TokenType::kNew, "New" },
  { TokenType::kNone, "None" },
  { TokenType::kParent, "Parent" },
  { TokenType::kProperty, "Property" },
  { TokenType::kReturn, "Return" },
  { TokenType::kScriptName, "ScriptName" },
  { TokenType::kSelf, "Self" },
  { TokenType::kState, "State" },
  { TokenType::kString, "String" },
  { TokenType::kTrue, "True" },
  { TokenType::kWhile, "While" },
  { TokenType::kConst, "Const" },
  { TokenType::kEndPropertyGroup, "EndPropertyGroup" },
  { TokenType::kEndStruct, "EndStruct" },
  { TokenType::kIs, "Is" },
  { TokenType::kPropertyGroup, "PropertyGroup" },
  { TokenType::kStruct, "Struct" },
  { TokenType::kVar, "Var" },
};

const std::string PapyrusLexer::Token::prettyTokenType(TokenType tp) {
  auto f = prettyTokenTypeNameMap.find(tp);
  if (f == prettyTokenTypeNameMap.end())
    CapricaError::logicalFatal("Unable to determine the pretty form of token type %i!", (int32_t)tp);
  return f->second;
}

void PapyrusLexer::setTok(TokenType tp, int consumeChars) {
  cur = Token(tp, location);
  for (int i = 0; i < consumeChars; i++)
    strm.get();
}

void PapyrusLexer::setTok(Token& tok) {
  cur = tok;
  cur.location = location;
}

PapyrusLexer::Token PapyrusLexer::peekToken(int distance) {
  auto oldCur = cur;
  auto oldLoc = location;
  auto oldPos = strm.tellg();

  for (int i = 0; i <= distance; i++)
    consume();

  auto newTok = cur;
  cur = oldCur;
  location = oldLoc;
  strm.seekg(oldPos);
  return newTok;
}

static const std::map<std::string, TokenType, CaselessStringComparer> keywordMap {
  { "as", TokenType::kAs },
  { "auto", TokenType::kAuto },
  { "autoreadonly", TokenType::kAutoReadOnly },
  { "bool", TokenType::kBool },
  { "else", TokenType::kElse },
  { "elseif", TokenType::kElseIf },
  { "endevent", TokenType::kEndEvent },
  { "endfunction", TokenType::kEndFunction },
  { "endif", TokenType::kEndIf },
  { "endproperty", TokenType::kEndProperty },
  { "endstate", TokenType::kEndState },
  { "endwhile", TokenType::kEndWhile },
  { "event", TokenType::kEvent },
  { "extends", TokenType::kExtends },
  { "false", TokenType::kFalse },
  { "float", TokenType::kFloat },
  { "function", TokenType::kFunction },
  { "global", TokenType::kGlobal },
  { "if", TokenType::kIf },
  { "import", TokenType::kImport },
  { "int", TokenType::kInt },
  { "length", TokenType::kLength },
  { "native", TokenType::kNative },
  { "new", TokenType::kNew },
  { "none", TokenType::kNone },
  { "parent", TokenType::kParent },
  { "property", TokenType::kProperty },
  { "return", TokenType::kReturn },
  { "scriptname", TokenType::kScriptName },
  { "self", TokenType::kSelf },
  { "state", TokenType::kState },
  { "string", TokenType::kString },
  { "true", TokenType::kTrue },
  { "while", TokenType::kWhile },
};

// Additional speculative keywords for FO4
static const std::map<std::string, TokenType, CaselessStringComparer> speculativeKeywordMap {
  { "const", TokenType::kConst },
  { "endpropertygroup", TokenType::kEndPropertyGroup },
  { "endstruct", TokenType::kEndStruct },
  { "is", TokenType::kIs },
  { "propertygroup", TokenType::kPropertyGroup },
  { "struct", TokenType::kStruct },
  { "var", TokenType::kVar },
};

void PapyrusLexer::consume() {
StartOver:
  auto c = strm.get();
  
  switch (c) {
    case -1:
      return setTok(TokenType::END);
    case '(':
      return setTok(TokenType::LParen);
    case ')':
      return setTok(TokenType::RParen);
    case '[':
      return setTok(TokenType::LSquare);
    case ']':
      return setTok(TokenType::RSquare);
    case '.':
      return setTok(TokenType::Dot);
    case ',':
      return setTok(TokenType::Comma);

    case '=':
      if (strm.peek() == '=')
        return setTok(TokenType::CmpEq, 1);
      return setTok(TokenType::Equal);
    case '!':
      if (strm.peek() == '=')
        return setTok(TokenType::CmpNeq, 1);
      return setTok(TokenType::Exclaim);
    case '+':
      if (strm.peek() == '=')
        return setTok(TokenType::PlusEqual, 1);
      return setTok(TokenType::Plus);
    case '-':
      if (strm.peek() == '=')
        return setTok(TokenType::MinusEqual, 1);
      if (isdigit(strm.peek()))
        goto Number;
      return setTok(TokenType::Minus);
    case '*':
      if (strm.peek() == '=')
        return setTok(TokenType::MulEqual, 1);
      return setTok(TokenType::Mul);
    case '/':
      if (strm.peek() == '=')
        return setTok(TokenType::DivEqual, 1);
      return setTok(TokenType::Div);
    case '%':
      if (strm.peek() == '=')
        return setTok(TokenType::ModEqual, 1);
      return setTok(TokenType::Mod);
    case '<':
      if (strm.peek() == '=')
        return setTok(TokenType::CmpLte, 1);
      return setTok(TokenType::CmpLt);
    case '>':
      if (strm.peek() == '=')
        return setTok(TokenType::CmpGte, 1);
      return setTok(TokenType::CmpGt);

    case '|':
      if (strm.peek() != '|')
        CapricaError::fatal(location, "Bitwise OR is unsupported. Did you intend to use a logical or (\"||\") instead?");
      return setTok(TokenType::BooleanOr, 1);
    case '&':
      if (strm.peek() != '&')
        CapricaError::fatal(location, "Bitwise AND is unsupported. Did you intend to use a logical and (\"&&\") instead?");
      return setTok(TokenType::BooleanAnd, 1);

    Number:
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
      if (c == '0' && strm.peek() == 'x') {
        str.put(strm.get());
        while (isxdigit(strm.peek()))
          str.put(strm.get());
        
        auto i = std::stoul(str.str(), nullptr, 16);
        auto tok = Token(TokenType::Integer, location);
        tok.iValue = (int32_t)i;
        return setTok(tok);
      }

      // Either normal int or float.
      while (isdigit(strm.peek()))
        str.put(strm.get());

      // It's a float.
      if (strm.peek() == '.') {
        str.put(strm.get());
        while (isdigit(strm.peek()))
          str.put(strm.get());

        // Allow e+ notation.
        if (CapricaConfig::enableLanguageExtensions && strm.peek() == 'e') {
          str.put(strm.get());
          if (strm.get() != '+')
            CapricaError::fatal(location, "Unexpected character 'e'!");
          str.put('+');

          while (isdigit(strm.peek()))
            str.put(strm.get());
        }

        auto f = std::stof(str.str());
        auto tok = Token(TokenType::Float, location);
        tok.fValue = f;
        return setTok(tok);
      }

      auto s = str.str();
      if (s.size() < 8 || (s.size() == 8 && s[0] <= '4')) {
        // It is probably an integer, but maybe not.
        try {
          auto i = std::stoul(s);
          auto tok = Token(TokenType::Integer, location);
          tok.iValue = (int32_t)i;
          return setTok(tok);
        } catch (std::out_of_range oor) { }
      }
      // It's very definitely a float, and a very large one at that.
      auto f = std::stof(s);
      auto tok = Token(TokenType::Float, location);
      tok.fValue = f;
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
      if (c == ':') {
        if (!CapricaConfig::allowCompilerIdentifiers || strm.peek() != ':')
          CapricaError::fatal(location, "Unexpected character '%c'!", (char)c);
        strm.get();
        str.put(':');
        str.put(':');
      } else {
        str.put(c);
      }

      while (isalnum(strm.peek()) || strm.peek() == '_')
        str.put(strm.get());

      if (CapricaConfig::enableDecompiledStructNameRefs && strm.peek() == '#') {
        str.put(strm.get());

        while (isalnum(strm.peek()) || strm.peek() == '_')
          str.put(strm.get());
      }

      auto ident = str.str();
      auto f = keywordMap.find(ident);
      if (f != keywordMap.end())
        return setTok(f->second);

      if (CapricaConfig::enableSpeculativeSyntax) {
        auto f2 = speculativeKeywordMap.find(ident);
        if (f2 != speculativeKeywordMap.end())
          return setTok(f2->second);
      }

      auto tok = Token(TokenType::Identifier, location);
      tok.sValue = ident;
      return setTok(tok);
    }

    case '"':
    {
      std::ostringstream str;

      while (strm.peek() != '"' && strm.peek() != '\r' && strm.peek() != '\n' && strm.peek() != -1) {
        if (strm.peek() == '\\') {
          strm.get();
          auto escapeChar = strm.get();
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
          str.put(strm.get());
        }
      }

      if (strm.peek() != '"')
        CapricaError::fatal(location, "Unclosed string!");
      strm.get();

      auto tok = Token(TokenType::String, location);
      tok.sValue = str.str();
      return setTok(tok);
    }

    case ';':
    {
      if (strm.peek() == '/') {
        // Multiline comment.
        strm.get();

        while (strm.peek() != -1) {
          if (strm.peek() == '\r' || strm.peek() == '\n') {
            auto c2 = strm.get();
            if (c2 == '\r' && strm.peek() == '\n')
              strm.get();
            location.line++;
          }

          if (strm.get() == '/' && strm.peek() == ';') {
            strm.get();
            goto StartOver;
          }
        }

        CapricaError::fatal(location, "Unexpected EOF before the end of a multiline comment!");
      }

      // Single line comment.
      while (strm.peek() != '\r' && strm.peek() != '\n' && strm.peek() != -1)
        strm.get();
      goto StartOver;
    }

    case '{':
    {
      std::ostringstream str;

      // Trim all leading whitespace.
      while (isspace(strm.peek()))
        strm.get();

      while (strm.peek() != '}' && strm.peek() != -1) {
        // For sanity reasons, we only put out unix newlines in the
        // doc comment string.
        auto c2 = strm.get();
        if (c2 == '\r' && strm.peek() == '\n') {
          strm.get();
          str.put('\n');
          location.line++;
        } else {
          if (c2 == '\n')
            location.line++;
          // Whether this is a Unix newline, or a normal character,
          // we don't care, they both get written as-is.
          str.put(c2);
        }
      }

      if (strm.peek() == -1)
        CapricaError::fatal(location, "Unexpected EOF before the end of a documentation comment!");
      strm.get();

      auto tok = Token(TokenType::DocComment, location);
      tok.sValue = str.str();
      // Trim trailing whitespace.
      if (tok.sValue.length())
        tok.sValue = tok.sValue.substr(0, tok.sValue.find_last_not_of(" \t\n\v\f\r") + 1);
      return setTok(tok);
    }

    case '\\':
    {
      auto prevLoc = location;
      consume();
      if (cur.type != TokenType::EOL)
        CapricaError::fatal(prevLoc, "Unexpected '\\'! Division is done with a forward slash '/'.");
      goto StartOver;
    }

    case '\r':
    case '\n':
    {
      if (c == '\r' && strm.peek() == '\n')
        strm.get();
      location.line++;
      return setTok(TokenType::EOL);
    }

    case ' ':
    case '\t':
    {
      while (strm.peek() == ' ' || strm.peek() == '\t')
        strm.get();
      goto StartOver;
    }

    default:
      CapricaError::fatal(location, "Unexpected character '%c'!", (char)c);
  }
}

}}}
