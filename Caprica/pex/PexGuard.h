#pragma once

#include <common/IntrusiveLinkedList.h>

#include <pex/PexAsmWriter.h>
#include <pex/PexFunction.h>
#include <pex/PexReader.h>
#include <pex/PexString.h>
#include <pex/PexUserFlags.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexFile;
struct PexObject;

struct PexGuard final
{
  PexString name{ };

  explicit PexGuard() = default;
  PexGuard(const PexGuard&) = delete;
  ~PexGuard() = default;

  static PexGuard* read(allocators::ChainedPool* alloc, PexReader& rdr);
  void write(PexWriter& wtr) const;
  void writeAsm(const PexFile* file, PexAsmWriter& wtr) const;

private:
  friend IntrusiveLinkedList<PexGuard>;
  PexGuard* next{ nullptr };
};

}}
