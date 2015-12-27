#pragma once

#include <string>

#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/statements/PapyrusStatement.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusDeclareStatement final : public PapyrusStatement
{
  PapyrusType type{ };
  std::string name{ "" };
  expressions::PapyrusExpression* initialValue{ nullptr };

  PapyrusDeclareStatement() = default;
  ~PapyrusDeclareStatement() {
    if (initialValue)
      delete initialValue;
  }
};

}}}
