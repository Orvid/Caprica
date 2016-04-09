#include <pex/parser/PexAsmLexer.h>

#include <cctype>
#include <map>
#include <unordered_map>

#include <common/CaselessStringComparer.h>

namespace caprica { namespace pex { namespace parser {

static const std::unordered_map<TokenType, const std::string> prettyTokenTypeNameMap{
  { TokenType::Unknown, "Unknown" },
  { TokenType::EOL, "EOL" },
  { TokenType::END, "EOF" },
  { TokenType::Identifier, "Identifier" },
  { TokenType::String, "String" },
  { TokenType::Integer, "Integer" },
  { TokenType::Float, "Float" },
  { TokenType::LineNumer, ";@line" },

  { TokenType::kAutoState, ".autoState" },
  { TokenType::kAutoVar, ".autoVar" },
  { TokenType::kCode, ".code" },
  { TokenType::kCompileTime, ".compileTime" },
  { TokenType::kComputer, ".computer" },
  { TokenType::kDocString, ".docString" },
  { TokenType::kEndCode, ".endCode" },
  { TokenType::kEndFunction, ".endFunction" },
  { TokenType::kEndInfo, ".endInfo" },
  { TokenType::kEndLocalTable, ".endLocalTable" },
  { TokenType::kEndObject, ".endObject" },
  { TokenType::kEndObjectTable, ".endObjectTable" },
  { TokenType::kEndParamTable, ".endParamTable" },
  { TokenType::kEndProperty, ".endProperty" },
  { TokenType::kEndPropertyTable, ".endPropertyTable" },
  { TokenType::kEndPropertyGroup, ".endPropertyGroup" },
  { TokenType::kEndPropertyGroupTable, ".endPropertyGroupTable" },
  { TokenType::kEndState, ".endState" },
  { TokenType::kEndStateTable, ".endStateTable" },
  { TokenType::kEndStruct, ".endStruct" },
  { TokenType::kEndStructTable, ".endStructTable" },
  { TokenType::kEndUserFlagsRef, ".endUserFlagsRef" },
  { TokenType::kEndVariable, ".endVariable" },
  { TokenType::kEndVariableTable, ".endVariableTable" },
  { TokenType::kFlag, ".flag" },
  { TokenType::kFunction, ".function" },
  { TokenType::kInfo, ".info" },
  { TokenType::kInitialValue, ".initialValue" },
  { TokenType::kLocal, ".local" },
  { TokenType::kLocalTable, ".localTable" },
  { TokenType::kModifyTime, ".modifyTime" },
  { TokenType::kObject, ".object" },
  { TokenType::kObjectTable, ".objectTable" },
  { TokenType::kParam, ".param" },
  { TokenType::kParamTable, ".paramTable" },
  { TokenType::kProperty, ".property" },
  { TokenType::kPropertyTable, ".propertyTable" },
  { TokenType::kPropertyGroup, ".propertyGroup" },
  { TokenType::kPropertyGroupTable, ".propertyGroupTable" },
  { TokenType::kReturn, ".return" },
  { TokenType::kSource, ".source" },
  { TokenType::kState, ".state" },
  { TokenType::kStateTable, ".stateTable" },
  { TokenType::kStruct, ".struct" },
  { TokenType::kStructTable, ".structTable" },
  { TokenType::kUser, ".user" },
  { TokenType::kUserFlags, ".userFlags" },
  { TokenType::kUserFlagsRef, ".userFlagsRef" },
  { TokenType::kVariable, ".variable" },
  { TokenType::kVariableTable, ".variableTable" },
};

const std::string PexAsmLexer::Token::prettyTokenType(TokenType tp) {
  auto f = prettyTokenTypeNameMap.find(tp);
  if (f == prettyTokenTypeNameMap.end())
    CapricaError::logicalFatal("Unable to determine the pretty form of token type %i!", (int32_t)tp);
  return f->second;
}

void PexAsmLexer::setTok(TokenType tp, const CapricaFileLocation& loc) {
  cur = Token(tp, loc);
}

void PexAsmLexer::setTok(Token& tok) {
  cur = tok;
}

static const std::map<const std::string, TokenType, CaselessStringComparer> dotIdentifierMap{
  { "autostate", TokenType::kAutoState },
  { "autovar", TokenType::kAutoVar },
  { "code", TokenType::kCode },
  { "compiletime", TokenType::kCompileTime },
  { "computer", TokenType::kComputer },
  { "docstring", TokenType::kDocString },
  { "endcode", TokenType::kEndCode },
  { "endfunction", TokenType::kEndFunction },
  { "endinfo", TokenType::kEndInfo },
  { "endlocaltable", TokenType::kEndLocalTable },
  { "endobject", TokenType::kEndObject },
  { "endobjecttable", TokenType::kEndObjectTable },
  { "endparamtable", TokenType::kEndParamTable },
  { "endproperty", TokenType::kEndProperty },
  { "endpropertytable", TokenType::kEndPropertyTable },
  { "endpropertygroup", TokenType::kEndPropertyGroup },
  { "endpropertygrouptable", TokenType::kEndPropertyGroupTable },
  { "endstate", TokenType::kEndState },
  { "endstatetable", TokenType::kEndStateTable },
  { "endstruct", TokenType::kEndStruct },
  { "endstructtable", TokenType::kEndStructTable },
  { "enduserflagsref", TokenType::kEndUserFlagsRef },
  { "endvariable", TokenType::kEndVariable },
  { "endvariabletable", TokenType::kEndVariableTable },
  { "flag", TokenType::kFlag },
  { "function", TokenType::kFunction },
  { "info", TokenType::kInfo },
  { "initialvalue", TokenType::kInitialValue },
  { "local", TokenType::kLocal },
  { "localtable", TokenType::kLocalTable },
  { "modifytime", TokenType::kModifyTime },
  { "object", TokenType::kObject },
  { "objecttable", TokenType::kObjectTable },
  { "param", TokenType::kParam },
  { "paramtable", TokenType::kParamTable },
  { "property", TokenType::kProperty },
  { "propertytable", TokenType::kPropertyTable },
  { "propertygroup", TokenType::kPropertyGroup },
  { "propertygrouptable", TokenType::kPropertyGroupTable },
  { "return", TokenType::kReturn },
  { "source", TokenType::kSource },
  { "state", TokenType::kState },
  { "statetable", TokenType::kStateTable },
  { "struct", TokenType::kStruct },
  { "structtable", TokenType::kStructTable },
  { "user", TokenType::kUser },
  { "userflags", TokenType::kUserFlags },
  { "userflagsref", TokenType::kUserFlagsRef },
  { "variable", TokenType::kVariable },
  { "variabletable", TokenType::kVariableTable },
};

void PexAsmLexer::consume() {
StartOver:
  auto baseLoc = location;
  auto c = getChar();
  
  switch (c) {
    case -1:
      return setTok(TokenType::END, baseLoc);

    case '.':
    {
      std::ostringstream str;
      while (isalpha(peekChar()))
        str.put(getChar());
      auto ident = str.str();
      auto f = dotIdentifierMap.find(ident);
      if (f == dotIdentifierMap.end())
        CapricaError::fatal(baseLoc, "Unknown directive '.%s'!", ident.c_str());
      return setTok(f->second, baseLoc);
    }

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
        auto tok = Token(TokenType::Integer, baseLoc);
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
        auto tok = Token(TokenType::Float, baseLoc);
        tok.fValue = f;
        return setTok(tok);
      }

      auto i = std::stoull(str.str());
      auto tok = Token(TokenType::Integer, baseLoc);
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
      auto tok = Token(TokenType::Identifier, baseLoc);
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

      auto tok = Token(TokenType::String, baseLoc);
      tok.sValue = str.str();
      return setTok(tok);
    }

    case ';':
    {
      if (getChar() != '@') {
        while (peekChar() != '\r' && peekChar() != '\n' && peekChar() != -1)
          getChar();
        goto StartOver;
      }
      if (getChar() != 'l' || getChar() != 'i' || getChar() != 'n' || getChar() != 'e')
        CapricaError::fatal(location, "Unexpected character sequence that looked line a ;@line directive!");
      return setTok(TokenType::LineNumer, baseLoc);
    }

    case '\r':
    case '\n':
    {
      if (c == '\r' && peekChar() == '\n')
        getChar();
      location.nextLine();
      return setTok(TokenType::EOL, baseLoc);
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
