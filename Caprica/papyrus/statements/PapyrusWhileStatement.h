#pragma once

#include <vector>

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusWhileStatement final : public PapyrusStatement
{
  expressions::PapyrusExpression* condition{ nullptr };
  std::vector<PapyrusStatement*> body{ };

  PapyrusWhileStatement() = default;
  ~PapyrusWhileStatement() {
    if (condition)
      delete condition;
    for (auto s : body)
      delete s;
  }
};

}}}
