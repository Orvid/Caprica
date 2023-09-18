#pragma once

#include <common/IntrusiveLinkedList.h>

#include <pex/PexAsmWriter.h>
#include <pex/PexFunction.h>
#include <pex/PexReader.h>
#include <pex/PexString.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexFile;
struct PexObject;

struct PexState final
{
  PexString name{ };
  IntrusiveLinkedList<PexFunction> functions{ };

  explicit PexState() = default;
  PexState(const PexState&) = delete;
  ~PexState() = default;

  static PexState *read(allocators::ChainedPool *alloc, PexReader &rdr, GameID gameType);
  void write(PexWriter& wtr) const;
  void writeAsm(const PexFile* file, const PexObject* obj, PexAsmWriter& wtr) const;

private:
  friend IntrusiveLinkedList<PexState>;
  PexState* next{ nullptr };
};

}}
