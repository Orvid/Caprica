#pragma once

#include <papyrus/expressions/PapyrusBinaryOpExpression.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusAssignStatement final : public PapyrusStatement
{
  expressions::PapyrusExpression* lValue{ nullptr };
  expressions::PapyrusBinaryOperatorType op{ expressions::PapyrusBinaryOperatorType::None };
  expressions::PapyrusExpression* rValue{ nullptr };

  PapyrusAssignStatement() = default;
  ~PapyrusAssignStatement() {
    if (lValue)
      delete lValue;
    if (rValue)
      delete rValue;
  }
};

}}}
