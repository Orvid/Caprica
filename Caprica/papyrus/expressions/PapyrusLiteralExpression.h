#pragma once

#include <papyrus/PapyrusValue.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/parser/PapyrusFileLocation.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusLiteralExpression final : public PapyrusExpression
{
  PapyrusValue value{ };

  PapyrusLiteralExpression(parser::PapyrusFileLocation loc) : PapyrusExpression(loc) { }
  ~PapyrusLiteralExpression() = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    bldr << location;
    return value.buildPex(file);
  }
};

}}}
