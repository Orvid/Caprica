#pragma once

#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/parser/PapyrusFileLocation.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusStatement abstract
{
  parser::PapyrusFileLocation location{ };

  PapyrusStatement(parser::PapyrusFileLocation loc) : location(loc) { }
  ~PapyrusStatement() = default;

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const abstract;
  virtual void semantic(PapyrusResolutionContext* ctx) abstract;
};

}}}
