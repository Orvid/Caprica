#include <papyrus/parser/PapyrusParser.h>

namespace caprica { namespace papyrus { namespace parser {

PapyrusScript* PapyrusParser::parseScript() {
  auto script = new PapyrusScript();

  while (cur.type != TokenType::END) {
    script->name += cur.asString() + "\r\n";
    consume();
  }
  return script;
}

}}}
