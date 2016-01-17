#pragma once

#include <common/CapricaFileLocation.h>
#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/statements/PapyrusStatementVisitor.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusStatement abstract
{
  const CapricaFileLocation location;

  explicit PapyrusStatement(const CapricaFileLocation& loc) : location(loc) { }
  PapyrusStatement(const PapyrusStatement&) = delete;
  virtual ~PapyrusStatement() = default;

  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const abstract;
  virtual void semantic(PapyrusResolutionContext* ctx) abstract;
  virtual void visit(PapyrusStatementVisitor& visitor) abstract;
};

}}}
