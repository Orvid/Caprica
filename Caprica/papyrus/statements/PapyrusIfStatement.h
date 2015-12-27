#pragma once

#include <utility>
#include <vector>

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusIfStatement final : public PapyrusStatement
{
  std::vector<std::pair<expressions::PapyrusExpression*, std::vector<PapyrusStatement*>>> ifBodies{ };
  std::vector<PapyrusStatement*> elseStatements{ };

  PapyrusIfStatement() = default;
  ~PapyrusIfStatement() {
    for (auto& i : ifBodies) {
      delete i.first;
      for (auto s : i.second)
        delete s;
    }
    for (auto s : elseStatements)
      delete s;
  }
};

}}}
