#pragma once

#include <common/IntrusiveLinkedList.h>

#include <pex/PexAsmWriter.h>
#include <pex/PexReader.h>
#include <pex/PexString.h>
#include <pex/PexUserFlags.h>
#include <pex/PexValue.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexFile;

struct PexStructMember final {
  PexString name {};
  PexString typeName {};
  PexUserFlags userFlags {};
  PexValue defaultValue {};
  bool isConst { false };
  PexString documentationString {};

  explicit PexStructMember() = default;
  PexStructMember(const PexStructMember&) = delete;
  ~PexStructMember() = default;

  static PexStructMember* read(allocators::ChainedPool* alloc, PexReader& rdr);
  void write(PexWriter& wtr) const;
  void writeAsm(const PexFile* file, PexAsmWriter& wtr) const;

private:
  friend IntrusiveLinkedList<PexStructMember>;
  PexStructMember* next { nullptr };
};

}}
