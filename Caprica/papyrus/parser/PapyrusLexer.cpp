#include <papyrus/parser/PapyrusLexer.h>

#include <cctype>
#include <map>

namespace caprica { namespace papyrus { namespace parser {

void PapyrusLexer::setTok(TokenType tp, int consumeChars) {
  cur = Token(tp);
  cur.line = lineNum;
  for (int i = 0; i < consumeChars; i++)
    strm.get();
}

void PapyrusLexer::setTok(Token& tok) {
  cur = tok;
  cur.line = lineNum;
}

static std::map<std::string, TokenType, CaselessStringComparer> keywordMap {
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

  // Additional speculative keywords for FO4
  { "const", TokenType::kConst },
  { "endpropertygroup", TokenType::kEndPropertyGroup },
  { "endstruct", TokenType::kEndStruct },
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
      return setTok(TokenType::RSqaure);
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
        fatalError("Bitwise OR is unsupported. Did you intend to use a logical or (\"||\") instead?");
      return setTok(TokenType::BooleanOr, 1);
    case '&':
      if (strm.peek() != '&')
        fatalError("Bitwise AND is unsupported. Did you intend to use a logical and (\"&&\") instead?");
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
        auto tok = Token(TokenType::Integer);
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

        auto f = std::stof(str.str());
        auto tok = Token(TokenType::Float);
        tok.fValue = f;
        return setTok(tok);
      }

      // It's an integer.
      auto i = std::stoul(str.str());
      auto tok = Token(TokenType::Integer);
      tok.iValue = (int32_t)i;
      return setTok(tok);
    }

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

      while (isalnum(strm.peek()) || strm.peek() == '_')
        str.put(strm.get());

      auto ident = str.str();
      auto f = keywordMap.find(ident);
      if (f != keywordMap.end())
        return setTok(f->second);

      auto tok = Token(TokenType::Identifier);
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
              fatalError("Unexpected EOF before the end of the string.");
            default:
              fatalError((std::string)"Unrecognized escape sequence: '\\" + (char)escapeChar + "'");
          }
        } else {
          str.put(strm.get());
        }
      }

      if (strm.peek() != '"')
        fatalError("Unclosed string!");
      strm.get();

      auto tok = Token(TokenType::String);
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
            lineNum++;
          }

          if (strm.get() == '/' && strm.peek() == ';') {
            strm.get();
            goto StartOver;
          }
        }

        fatalError("Unexpected EOF before the end of a multiline comment!");
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
          lineNum++;
        } else {
          if (c2 == '\n')
            lineNum++;
          // Whether this is a Unix newline, or a normal character,
          // we don't care, they both get written as-is.
          str.put(c2);
        }
      }

      if (strm.peek() == -1)
        fatalError("Unexpected EOF before the end of a documentation comment!");
      strm.get();

      auto tok = Token(TokenType::DocComment);
      tok.sValue = str.str();
      // Trim trailing whitespace.
      if (tok.sValue.length())
        tok.sValue = tok.sValue.substr(0, tok.sValue.find_last_not_of(" \t\n\v\f\r") + 1);
      return setTok(tok);
    }

    case '\\':
    {
      consume();
      if (cur.type != TokenType::EOL)
        fatalError("Unexpected '\'! Division is done with a forward slash '/'.");
      goto StartOver;
    }

    case '\r':
    case '\n':
    {
      if (c == '\r' && strm.peek() == '\n')
        strm.get();
      lineNum++;
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
      fatalError((std::string)"Unexpected character '" + (char)c + "'!");
  }
}

}}}
