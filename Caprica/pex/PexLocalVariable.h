#pragma once

#include <common/allocators/ChainedPool.h>
#include <common/IntrusiveLinkedList.h>
#include <common/IntrusiveStack.h>

#include <pex/PexAsmWriter.h>
#include <pex/PexString.h>

namespace caprica { namespace pex {

struct PexFile;
struct PexReader;
struct PexWriter;

struct PexLocalVariable final {
  PexString name {};
  PexString type {};

  explicit PexLocalVariable() = default;
  PexLocalVariable(const PexLocalVariable&) = delete;
  ~PexLocalVariable() = default;

  static PexLocalVariable* read(allocators::ChainedPool* alloc, PexReader& rdr);
  void write(PexWriter& wtr) const;
  void writeAsm(const PexFile* file, PexAsmWriter& wtr) const;

private:
  friend IntrusiveLinkedList<PexLocalVariable>;
  PexLocalVariable* next { nullptr };
  friend IntrusiveStack<PexLocalVariable>;
  PexLocalVariable* nextInStack { nullptr };
};

}}
