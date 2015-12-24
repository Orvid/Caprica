#pragma once

#include <istream>

#include <papyrus/PapyrusScript.h>
#include <papyrus/parser/PapyrusLexer.h>

namespace caprica { namespace papyrus { namespace parser {

struct PapyrusParser : private PapyrusLexer
{
  PapyrusParser(std::istream& strm) : PapyrusLexer(strm) { }
  ~PapyrusParser() = default;

  PapyrusScript* parseScript();
};

}}}
