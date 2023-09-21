#pragma once

#include <cstdint>

#include <common/IntrusiveLinkedList.h>

#include <pex/PexAsmWriter.h>
#include <pex/PexGuard.h>
#include <pex/PexProperty.h>
#include <pex/PexReader.h>
#include <pex/PexState.h>
#include <pex/PexString.h>
#include <pex/PexStruct.h>
#include <pex/PexUserFlags.h>
#include <pex/PexVariable.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexFile;

struct PexObject final
{
  PexString name{ };
  PexString parentClassName{ };
  PexString documentationString{ };
  bool isConst{ false };
  PexUserFlags userFlags{ };
  PexString autoStateName{ };
  IntrusiveLinkedList<PexStruct> structs{ };
  IntrusiveLinkedList<PexVariable> variables{ };
  IntrusiveLinkedList<PexProperty> properties{ };
  IntrusiveLinkedList<PexState> states{ };
  IntrusiveLinkedList<PexGuard> guards{ };

  explicit PexObject() = default;
  PexObject(const PexObject&) = delete;
  ~PexObject() = default;

  static PexObject* read(allocators::ChainedPool* alloc, PexReader& rdr);
  void write(PexWriter& wtr) const;
  void writeAsm(const PexFile* file, PexAsmWriter& wtr) const;

private:
  friend IntrusiveLinkedList<PexObject>;
  PexObject* next{ nullptr };
};

}}
