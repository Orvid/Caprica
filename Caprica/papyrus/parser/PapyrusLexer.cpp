#include <papyrus/parser/PapyrusLexer.h>

#include <cassert>
#include <cctype>
#include <map>
#include <unordered_map>

#include <common/CapricaConfig.h>
#include <common/CapricaStats.h>
#include <common/CaselessStringComparer.h>
#include <common/LargelyBufferedString.h>

#include <nmmintrin.h>

namespace caprica { namespace papyrus { namespace parser {

#define MAX_INTEGER_DIGITS 10
static const std::unordered_map<TokenType, std::string_view> prettyTokenTypeNameMap {
  {TokenType::Unknown,           "Unknown"              },
  { TokenType::EOL,              "EOL"                  },
  { TokenType::END,              "EOF"                  },
  { TokenType::Identifier,       "Identifier"           },
  { TokenType::DocComment,       "Documentation Comment"},
  { TokenType::String,           "String"               },
  { TokenType::Integer,          "Integer"              },
  { TokenType::Float,            "Float"                },
  { TokenType::LParen,           "("                    },
  { TokenType::RParen,           ")"                    },
  { TokenType::LSquare,          "["                    },
  { TokenType::RSquare,          "]"                    },
  { TokenType::Dot,              "."                    },
  { TokenType::Comma,            ","                    },
  { TokenType::Equal,            "="                    },
  { TokenType::Exclaim,          "!"                    },
  { TokenType::Plus,             "+"                    },
  { TokenType::PlusEqual,        "+="                   },
  { TokenType::Minus,            "-"                    },
  { TokenType::MinusEqual,       "-="                   },
  { TokenType::Mul,              "*"                    },
  { TokenType::MulEqual,         "*="                   },
  { TokenType::Div,              "/"                    },
  { TokenType::DivEqual,         "/="                   },
  { TokenType::Mod,              "%"                    },
  { TokenType::ModEqual,         "%="                   },
  { TokenType::CmpEq,            "=="                   },
  { TokenType::CmpNeq,           "!="                   },
  { TokenType::CmpLt,            "<"                    },
  { TokenType::CmpLte,           "<="                   },
  { TokenType::CmpGt,            ">"                    },
  { TokenType::CmpGte,           ">="                   },
  { TokenType::BooleanOr,        "||"                   },
  { TokenType::BooleanAnd,       "&&"                   },
  { TokenType::kAs,              "As"                   },
  { TokenType::kAuto,            "Auto"                 },
  { TokenType::kAutoReadOnly,    "AutoReadOnly"         },
  { TokenType::kBool,            "Bool"                 },
  { TokenType::kElse,            "Else"                 },
  { TokenType::kElseIf,          "ElseIf"               },
  { TokenType::kEndEvent,        "EndEvent"             },
  { TokenType::kEndFunction,     "EndFunction"          },
  { TokenType::kEndIf,           "EndIf"                },
  { TokenType::kEndProperty,     "EndProperty"          },
  { TokenType::kEndState,        "EndState"             },
  { TokenType::kEndWhile,        "EndWhile"             },
  { TokenType::kEvent,           "Event"                },
  { TokenType::kExtends,         "Extends"              },
  { TokenType::kFalse,           "False"                },
  { TokenType::kFloat,           "Float"                },
  { TokenType::kFunction,        "Function"             },
  { TokenType::kGlobal,          "Global"               },
  { TokenType::kIf,              "If"                   },
  { TokenType::kImport,          "Import"               },
  { TokenType::kInt,             "Int"                  },
  { TokenType::kIs,              "Is"                   },
  { TokenType::kLength,          "Length"               },
  { TokenType::kNative,          "Native"               },
  { TokenType::kNew,             "New"                  },
  { TokenType::kNone,            "None"                 },
  { TokenType::kParent,          "Parent"               },
  { TokenType::kProperty,        "Property"             },
  { TokenType::kReturn,          "Return"               },
  { TokenType::kScriptName,      "ScriptName"           },
  { TokenType::kSelf,            "Self"                 },
  { TokenType::kState,           "State"                },
  { TokenType::kString,          "String"               },
  { TokenType::kTrue,            "True"                 },
  { TokenType::kWhile,           "While"                },

 // Fallout 4 / Fallout 76
  { TokenType::kBetaOnly,        "BetaOnly"             },
  { TokenType::kConst,           "Const"                },
  { TokenType::kCustomEvent,     "CustomEvent"          },
  { TokenType::kCustomEventName, "CustomEventName"      },
  { TokenType::kDebugOnly,       "DebugOnly"            },
  { TokenType::kEndGroup,        "EndGroup"             },
  { TokenType::kEndStruct,       "EndStruct"            },
  { TokenType::kGroup,           "Group"                },
  { TokenType::kScriptEventName, "ScriptEventName"      },
  { TokenType::kStruct,          "Struct"               },
  { TokenType::kVar,             "Var"                  },

 // Starfield
  { TokenType::kGuard,           "Guard"                },
  { TokenType::kEndGuard,        "EndGuard"             },
  { TokenType::kTryGuard,        "TryGuard"             },

 // language extensions
  { TokenType::kBreak,           "Break"                },
  { TokenType::kCase,            "Case"                 },
  { TokenType::kContinue,        "Continue"             },
  { TokenType::kDefault,         "Default"              },
  { TokenType::kDo,              "Do"                   },
  { TokenType::kEndFor,          "EndFor"               },
  { TokenType::kEndForEach,      "EndForEach"           },
  { TokenType::kEndSwitch,       "EndSwitch"            },
  { TokenType::kFor,             "For"                  },
  { TokenType::kForEach,         "ForEach"              },
  { TokenType::kIn,              "In"                   },
  { TokenType::kLoopWhile,       "LoopWhile"            },
  { TokenType::kStep,            "Step"                 },
  { TokenType::kSwitch,          "Switch"               },
  { TokenType::kTo,              "To"                   },
};

std::string_view PapyrusLexer::Token::prettyTokenType(TokenType tp) {
  auto f = prettyTokenTypeNameMap.find(tp);
  if (f == prettyTokenTypeNameMap.end())
    CapricaReportingContext::logicalFatal("Unable to determine the pretty form of token type {}!", (int32_t)tp);
  return f->second;
}

ALWAYS_INLINE
void PapyrusLexer::setTok(TokenType tp, CapricaFileLocation loc, int consumeChars) {
  cur.type = tp;
  cur.location = loc;
  cur.location.endOffset = location.startOffset + consumeChars;
  advanceChars(consumeChars);
}

TokenType PapyrusLexer::peekTokenType(int distance) {
  assert(distance >= 0);
  assert(distance <= MaxPeekedTokens - 1);

  // It's already been lexed, peek directly.
  if (peekedTokenI + distance < peekedTokenCount)
    return peekedTokens[peekedTokenI + distance].type;
  else
    assert(peekedTokenI == 0);

  Token oldCur = std::move(cur);
  for (int i = peekedTokenCount; i <= distance; i++) {
    CapricaStats::peekedTokenCount++;
    realConsume();
    peekedTokens[peekedTokenCount] = std::move(cur);
    peekedTokenCount++;
    assert(peekedTokenCount <= MaxPeekedTokens);
  }
  cur = std::move(oldCur);

  return peekedTokens[distance].type;
}

static const caseless_unordered_identifier_ref_map<TokenType> keywordMap {
  { "as",              TokenType::kAs             },
  { "auto",            TokenType::kAuto           },
  { "autoreadonly",    TokenType::kAutoReadOnly   },
  { "bool",            TokenType::kBool           },
  { "else",            TokenType::kElse           },
  { "elseif",          TokenType::kElseIf         },
  { "endevent",        TokenType::kEndEvent       },
  { "endfunction",     TokenType::kEndFunction    },
  { "endif",           TokenType::kEndIf          },
  { "endproperty",     TokenType::kEndProperty    },
  { "endstate",        TokenType::kEndState       },
  { "endwhile",        TokenType::kEndWhile       },
  { "event",           TokenType::kEvent          },
  { "extends",         TokenType::kExtends        },
  { "false",           TokenType::kFalse          },
  { "float",           TokenType::kFloat          },
  { "function",        TokenType::kFunction       },
  { "global",          TokenType::kGlobal         },
  { "if",              TokenType::kIf             },
  { "import",          TokenType::kImport         },
  { "int",             TokenType::kInt            },
  { "is",              TokenType::kIs             },
  { "length",          TokenType::kLength         },
  { "native",          TokenType::kNative         },
  { "new",             TokenType::kNew            },
  { "none",            TokenType::kNone           },
  { "parent",          TokenType::kParent         },
  { "property",        TokenType::kProperty       },
  { "return",          TokenType::kReturn         },
  { "scriptname",      TokenType::kScriptName     },
  { "self",            TokenType::kSelf           },
  { "state",           TokenType::kState          },
  { "string",          TokenType::kString         },
  { "true",            TokenType::kTrue           },
  { "while",           TokenType::kWhile          },

 // Fallout 4 / Fallout 76
  { "betaonly",        TokenType::kBetaOnly       },
  { "const",           TokenType::kConst          },
  { "customevent",     TokenType::kCustomEvent    },
  { "customeventname", TokenType::kCustomEventName},
  { "debugonly",       TokenType::kDebugOnly      },
  { "endgroup",        TokenType::kEndGroup       },
  { "endstruct",       TokenType::kEndStruct      },
  { "group",           TokenType::kGroup          },
  { "scripteventname", TokenType::kScriptEventName},
  { "struct",          TokenType::kStruct         },
  { "var",             TokenType::kVar            },

 // Starfield
  // TODO: Verify starfield syntax
  { "guard",           TokenType::kGuard          },
  { "endguard",        TokenType::kEndGuard       },
  { "tryguard",        TokenType::kTryGuard       },
};

// Language extension keywords
static const caseless_unordered_identifier_ref_map<TokenType> languageExtensionsKeywordMap {
  {"break",       TokenType::kBreak     },
  { "case",       TokenType::kCase      },
  { "continue",   TokenType::kContinue  },
  { "default",    TokenType::kDefault   },
  { "do",         TokenType::kDo        },
  { "endfor",     TokenType::kEndFor    },
  { "endforeach", TokenType::kEndForEach},
  { "endswitch",  TokenType::kEndSwitch },
  { "for",        TokenType::kFor       },
  { "foreach",    TokenType::kForEach   },
  { "in",         TokenType::kIn        },
  { "loopwhile",  TokenType::kLoopWhile },
  { "step",       TokenType::kStep      },
  { "switch",     TokenType::kSwitch    },
  { "to",         TokenType::kTo        },
};

// keywords which can never pass as identifiers
static const std::unordered_set<TokenType> nonIdentifiersSet {
  TokenType::kAuto, 
  TokenType::kAutoReadOnly, 
  TokenType::kBool, 
  TokenType::kConst,
  TokenType::kFalse, 
  TokenType::kFloat, 
  TokenType::kInt, 
  TokenType::kNone, 
  TokenType::kString, 
  TokenType::kStruct, 
  TokenType::kVar,
};

const bool keywordCanBeIdentifier(TokenType tp) {
  return keywordIsInGame(tp, conf::Papyrus::game, true) && nonIdentifiersSet.find(tp) == nonIdentifiersSet.end();
}

ALWAYS_INLINE
static bool isAsciiAlphaNumeric(int c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

ALWAYS_INLINE
static bool isAsciiDigit(int c) {
  return c >= '0' && c <= '9';
}

void PapyrusLexer::consume() {
  CapricaStats::consumedTokenCount++;
  if (peekedTokenCount) {
    cur = std::move(peekedTokens[peekedTokenI]);
    peekedTokenI++;
    if (peekedTokenI == peekedTokenCount) {
      peekedTokenI = 0;
      peekedTokenCount = 0;
    }
    return;
  }
  realConsume();
}

NEVER_INLINE
void PapyrusLexer::realConsume() {
StartOver:
  auto baseLoc = location;
  auto c = getChar();

  switch (c) {
    case -1:
      // Always pretend that there's an EOL at the end of the
      // file.
      if (cur.type == TokenType::EOL)
        return setTok(TokenType::END, baseLoc);
      return setTok(TokenType::EOL, baseLoc);
    case '(':
      return setTok(TokenType::LParen, baseLoc);
    case ')':
      return setTok(TokenType::RParen, baseLoc);
    case '[':
      return setTok(TokenType::LSquare, baseLoc);
    case ']':
      return setTok(TokenType::RSquare, baseLoc);
    case '.':
      return setTok(TokenType::Dot, baseLoc);
    case ',':
      return setTok(TokenType::Comma, baseLoc);

    case '=':
      if (peekChar() == '=')
        return setTok(TokenType::CmpEq, baseLoc, 1);
      return setTok(TokenType::Equal, baseLoc);
    case '!':
      if (peekChar() == '=')
        return setTok(TokenType::CmpNeq, baseLoc, 1);
      return setTok(TokenType::Exclaim, baseLoc);
    case '+':
      if (peekChar() == '=')
        return setTok(TokenType::PlusEqual, baseLoc, 1);
      return setTok(TokenType::Plus, baseLoc);
    case '-':
      if (peekChar() == '=')
        return setTok(TokenType::MinusEqual, baseLoc, 1);
      if (isAsciiDigit(peekChar()))
        goto Number;
      return setTok(TokenType::Minus, baseLoc);
    case '*':
      if (peekChar() == '=')
        return setTok(TokenType::MulEqual, baseLoc, 1);
      return setTok(TokenType::Mul, baseLoc);
    case '/':
      if (peekChar() == '=')
        return setTok(TokenType::DivEqual, baseLoc, 1);
      return setTok(TokenType::Div, baseLoc);
    case '%':
      if (peekChar() == '=')
        return setTok(TokenType::ModEqual, baseLoc, 1);
      return setTok(TokenType::Mod, baseLoc);
    case '<':
      if (peekChar() == '=')
        return setTok(TokenType::CmpLte, baseLoc, 1);
      return setTok(TokenType::CmpLt, baseLoc);
    case '>':
      if (peekChar() == '=')
        return setTok(TokenType::CmpGte, baseLoc, 1);
      return setTok(TokenType::CmpGt, baseLoc);

    case '|':
      if (peekChar() != '|') {
        reportingContext.error(baseLoc,
                               "Bitwise OR is unsupported. Did you intend to use a logical or (\"||\") instead?");
      }
      return setTok(TokenType::BooleanOr, baseLoc, 1);
    case '&':
      if (peekChar() != '&') {
        reportingContext.error(baseLoc,
                               "Bitwise AND is unsupported. Did you intend to use a logical and (\"&&\") instead?");
      }
      return setTok(TokenType::BooleanAnd, baseLoc, 1);

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
    case '9': {
      LargelyBufferedString str;
      str.push_back((char)c);

      // It's hex.
      if (c == '0' && (peekChar() == 'x' || peekChar() == 'X')) {
        str.push_back((char)getChar());
        while (isxdigit(peekChar()))
          str.push_back((char)getChar());

        str.push_back('\0');
        auto i = std::strtoul(str.data(), nullptr, 16);
        setTok(TokenType::Integer, baseLoc);
        cur.val.i = (int32_t)i;
        return;
      }

      // Either normal int or float.
      while (isAsciiDigit(peekChar()))
        str.push_back((char)getChar());

      // It's a float.
      if (peekChar() == '.') {
        str.push_back((char)getChar());
        while (isAsciiDigit(peekChar()))
          str.push_back((char)getChar());

        // Allow e+ notation.
        if (conf::Papyrus::enableLanguageExtensions && peekChar() == 'e') {
          str.push_back((char)getChar());
          if (getChar() != '+')
            reportingContext.error(location, "Unexpected character 'e'!");
          str.push_back('+');

          while (isAsciiDigit(peekChar()))
            str.push_back((char)getChar());
        }

        str.push_back('\0');
        auto f = std::strtof(str.data(), nullptr);
        setTok(TokenType::Float, baseLoc);
        cur.val.f = f;
        return;
      }

      if (str.size() < MAX_INTEGER_DIGITS || (str.size() == MAX_INTEGER_DIGITS && str.data()[0] <= '4')) {
        // It is probably an integer, but maybe not.
        try {
          str.push_back('\0');
          auto i = std::strtoul(str.data(), nullptr, 0);
          setTok(TokenType::Integer, baseLoc);
          cur.val.i = (int32_t)i;
          return;
        } catch (std::out_of_range oor) {
        }
      } else {
        str.push_back('\0');
      }
      // It's very definitely a float, and a very large one at that.
      auto f = std::strtof(str.data(), nullptr);
      setTok(TokenType::Float, baseLoc);
      cur.val.f = f;
      return;
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
    case 'Z': {
      const char* baseStrm = strm - 1;

      if (c == ':') {
        if (!conf::Papyrus::allowCompilerIdentifiers || peekChar() != ':')
          reportingContext.error(baseLoc, "Unexpected character '{}'!", (char)c);
        getChar();
      }

      static const __m128i identifierChars = {
        'a', 'z', 'A', 'Z', '0', '9', '_', '_', ':', ':', '\0',
      };

      int idx = 0;
      do {
        // Don't you just love SSE 4.2?
        // This scans 16 characters at a
        // time for the first one that
        // isn't in the identifier set.
        idx = _mm_cmpistri(identifierChars,
                           _mm_loadu_si128((__m128i*)strm),
                           _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_NEGATIVE_POLARITY | _SIDD_LEAST_SIGNIFICANT);
        advanceChars(idx);
      } while (idx == 16);

      if (conf::Papyrus::allowDecompiledStructNameRefs && peekChar() == '#') {
        getChar();

        while (isAsciiAlphaNumeric(peekChar()) || peekChar() == '_')
          getChar();
      }

      identifier_ref str { baseStrm, (size_t)(strm - baseStrm) };
      auto f = keywordMap.find(str);
      if (f != keywordMap.end() && keywordIsInGame(f->second, conf::Papyrus::game))
        return setTok(f->second, baseLoc);
      if (conf::Papyrus::enableLanguageExtensions) {
        auto f2 = languageExtensionsKeywordMap.find(str);
        if (f2 != languageExtensionsKeywordMap.end())
          return setTok(f2->second, baseLoc);
      }

      setTok(TokenType::Identifier, baseLoc);
      cur.val.s = str;
      return;
    }

    case '"': {
      const char* baseStrm = strm;
      size_t charsRequired = 0;

      while (peekChar() != '"' && peekChar() != '\r' && peekChar() != '\n' && peekChar() != -1) {
        if (peekChar() == '\\') {
          getChar();
          auto escapeChar = getChar();
          switch (escapeChar) {
            case 'n':
            case 't':
            case '\\':
            case '"':
              break;
            case -1:
              reportingContext.error(location, "Unexpected EOF before the end of the string.");
              continue;
            default:
              reportingContext.error(location, "Unrecognized escape sequence: '\\{}'", (char)escapeChar);
              charsRequired += 2;
              break;
          }
        } else {
          getChar();
        }
        charsRequired++;
      }
      identifier_ref str { baseStrm, (size_t)(strm - baseStrm) };

      if (peekChar() != '"')
        reportingContext.error(location, "Unclosed string!");
      else
        getChar();

      setTok(TokenType::String, baseLoc);
      if (charsRequired != str.size()) {
        auto buf = alloc->allocate(charsRequired);
        for (size_t i = 0, i2 = 0; i < charsRequired;) {
          if (baseStrm[i2] == '\\') {
            i2++;
            switch (baseStrm[i2++]) {
              case 'n':
                buf[i++] = '\n';
                break;
              case 't':
                buf[i++] = '\t';
                break;
              case '\\':
                buf[i++] = '\\';
                break;
              case '"':
                buf[i++] = '"';
                break;
              default:
                reportingContext.error(location, "Unrecognized escape sequence: '\\{}'", (char)baseStrm[i2 - 1]);
                buf[i++] = baseStrm[i2 - 2];
                buf[i++] = baseStrm[i2 - 1];
                break;
            }
          } else {
            buf[i++] = baseStrm[i2++];
          }
        }
        cur.val.s = identifier_ref(buf, charsRequired);
      } else {
        cur.val.s = alloc->allocateIdentifier(str.data(), str.size());
      }
      return;
    }

    case ';': {
      if (peekChar() == '/') {
        // Multiline comment.
        getChar();

        while (peekChar() != -1) {
          if (peekChar() == '\r' || peekChar() == '\n') {
            auto c2 = getChar();
            if (c2 == '\r' && peekChar() == '\n')
              getChar();
            reportingContext.pushNextLineOffset(location);
          }

          if (getChar() == '/' && peekChar() == ';') {
            getChar();
            goto StartOver;
          }
        }

        reportingContext.error(location, "Unexpected EOF before the end of a multiline comment!");
      }

      // Single line comment.
      while (peekChar() != '\r' && peekChar() != '\n' && peekChar() != -1)
        getChar();
      goto StartOver;
    }

    case '{': {
      // Trim all leading whitespace.
      while (isspace(peekChar()))
        getChar();

      const char* baseStrm = strm;
      size_t charsRequired = 0;
      while (peekChar() != '}' && peekChar() != -1) {
        // For sanity reasons, we only put out unix newlines in the
        // doc comment string.
        charsRequired++;
        auto c2 = getChar();
        if (c2 == '\r' && peekChar() == '\n') {
          getChar();
          reportingContext.pushNextLineOffset(location);
        } else {
          if (c2 == '\n')
            reportingContext.pushNextLineOffset(location);
          // Whether this is a Unix newline, or a normal character,
          // we don't care, they both get written as-is.
        }
      }
      identifier_ref str { baseStrm, (size_t)(strm - baseStrm) };

      if (peekChar() == -1)
        reportingContext.error(location, "Unexpected EOF before the end of a documentation comment!");
      else
        getChar(); // Consume final '}'

      setTok(TokenType::DocComment, baseLoc);
      // Trim trailing whitespace.
      auto lPos = str.find_last_not_of(" \t\n\v\f\r");
      if (lPos != identifier_ref::npos && lPos != str.size() + 1 && lPos + 1 != str.size()) {
        charsRequired -= str.size() - (lPos + 1);
        str = str.substr(0, lPos + 1);
      }

      if (charsRequired != str.size()) {
        auto buf = alloc->allocate(charsRequired);
        for (size_t i = 0, i2 = 0; i < charsRequired;) {
          if (baseStrm[i2] == '\r' && i2 + 1 < str.size() && baseStrm[i2 + 1] == '\n') {
            i2 += 2;
            buf[i++] = '\n';
          } else {
            buf[i++] = baseStrm[i2++];
          }
        }
        cur.val.s = identifier_ref(buf, charsRequired);
      } else {
        cur.val.s = alloc->allocateIdentifier(str.data(), str.size());
      }
      return;
    }

    case '\\': {
      consume();
      if (cur.type != TokenType::EOL)
        reportingContext.error(baseLoc, "Unexpected '\\'! Division is done with a forward slash '/'.");
      goto StartOver;
    }

    case '\r':
    case '\n': {
      if (c == '\r' && peekChar() == '\n')
        getChar();
      reportingContext.pushNextLineOffset(location);
      return setTok(TokenType::EOL, baseLoc);
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

}}}
