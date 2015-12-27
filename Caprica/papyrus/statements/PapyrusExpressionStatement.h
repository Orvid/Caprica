#pragma once

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusExpressionStatement final : public PapyrusStatement
{
  expressions::PapyrusExpression* expression{ nullptr };

  PapyrusExpressionStatement() = default;
  ~PapyrusExpressionStatement() {
    if (expression)
      delete expression;
  }
};

}}}
