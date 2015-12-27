#pragma once

#include <papyrus/parser/PapyrusFileLocation.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusExpression abstract
{
  parser::PapyrusFileLocation location{ };

  PapyrusExpression(parser::PapyrusFileLocation loc) : location(loc) { }
  ~PapyrusExpression() = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const abstract;
};

}}}
