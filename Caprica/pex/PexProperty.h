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

struct PexProperty final
{
  PexString name{ };
  PexString typeName{ };
  PexString documentationString{ };
  PexUserFlags userFlags{ };
  PexString autoVar{ };
  PexFunction* readFunction{ nullptr };
  PexFunction* writeFunction{ nullptr };
  bool isAuto{ false };
  bool isReadable{ false };
  bool isWritable{ false };

  explicit PexProperty() = default;
  PexProperty(const PexProperty&) = delete;
  ~PexProperty() = default;

  static PexProperty *read(allocators::ChainedPool *alloc, PexReader &rdr, GameID gameType);
  void write(PexWriter& wtr) const;
  void writeAsm(const PexFile* file, const PexObject* obj, PexAsmWriter& wtr) const;

private:
  friend IntrusiveLinkedList<PexProperty>;
  PexProperty* next{ nullptr };
};

}}
