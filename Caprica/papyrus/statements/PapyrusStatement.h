#pragma once

#include <common/CapricaFileLocation.h>
#include <common/IntrusiveLinkedList.h>

#include <papyrus/PapyrusCFG.h>
#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/statements/PapyrusStatementVisitor.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace statements {

struct PapyrusStatement abstract
{
  const CapricaFileLocation location;

  explicit PapyrusStatement(CapricaFileLocation loc) : location(loc) { }
  PapyrusStatement(const PapyrusStatement&) = delete;
  virtual ~PapyrusStatement() = default;

  virtual bool buildCFG(PapyrusCFG& cfg) const = 0;
  virtual void buildPex(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const = 0;
  virtual void semantic(PapyrusResolutionContext* ctx) = 0;
  virtual void semantic_skyrim_first_pass(PapyrusResolutionContext* ctx) {};
  virtual void visit(PapyrusStatementVisitor& visitor) = 0;

private:
  friend IntrusiveLinkedList<PapyrusStatement>;
  PapyrusStatement* next{ nullptr };
};

}}}
