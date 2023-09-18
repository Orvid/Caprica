#pragma once

#include <common/IntrusiveLinkedList.h>

#include <pex/PexAsmWriter.h>
#include <pex/PexReader.h>
#include <pex/PexString.h>
#include <pex/PexStructMember.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexFile;

struct PexStruct final
{
  PexString name{ };
  IntrusiveLinkedList<PexStructMember> members{ };

  explicit PexStruct() = default;
  PexStruct(const PexStruct&) = delete;
  ~PexStruct() = default;

  static PexStruct *read(allocators::ChainedPool *alloc, PexReader &rdr, GameID gameType);
  void write(PexWriter& wtr) const;
  void writeAsm(const PexFile* file, PexAsmWriter& wtr) const;

private:
  friend IntrusiveLinkedList<PexStruct>;
  PexStruct* next{ nullptr };
};

}}
