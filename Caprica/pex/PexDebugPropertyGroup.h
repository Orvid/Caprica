#pragma once

#include <common/IntrusiveLinkedList.h>

#include <pex/PexReader.h>
#include <pex/PexString.h>
#include <pex/PexUserFlags.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexDebugPropertyGroup final {
  PexString objectName {};
  PexString groupName {};
  PexString documentationString {};
  PexUserFlags userFlags {};
  IntrusiveLinkedList<IntrusivePexString> properties {};

  explicit PexDebugPropertyGroup() = default;
  PexDebugPropertyGroup(const PexDebugPropertyGroup&) = delete;
  ~PexDebugPropertyGroup() = default;

  static PexDebugPropertyGroup* read(allocators::ChainedPool* alloc, PexReader& rdr);
  void write(PexWriter& wtr) const;
  void writeAsm(const PexFile* file, PexAsmWriter& wtr) const;

private:
  friend IntrusiveLinkedList<PexDebugPropertyGroup>;
  PexDebugPropertyGroup* next { nullptr };
};

}}
