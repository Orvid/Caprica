#pragma once

#include <cstdint>
#include <ctime>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <common/allocators/ChainedPool.h>
#include <common/allocators/ReffyStringPool.h>
#include <common/identifier_ref.h>
#include <common/IntrusiveLinkedList.h>

#include <pex/PexAsmWriter.h>
#include <pex/PexDebugInfo.h>
#include <pex/PexObject.h>
#include <pex/PexReader.h>
#include <pex/PexString.h>
#include <pex/PexWriter.h>

namespace caprica {

namespace papyrus { struct PapyrusScript; }

namespace pex {

struct PexFile final
{
  allocators::ChainedPool* alloc;

  uint8_t majorVersion{ 3 };
  uint8_t minorVersion{ 9 };
  uint16_t gameID{ 2 }; // Default to Fallout 4
  time_t compilationTime{ };
  std::string sourceFileName{ "" };
  std::string userName{ "" };
  std::string computerName{ "" };
  PexDebugInfo* debugInfo{ nullptr };
  IntrusiveLinkedList<PexObject> objects{ };

  explicit PexFile(allocators::ChainedPool* p);
  PexFile(const PexFile&) = delete;

  void ensureDebugInfo() {
    if (!debugInfo)
      debugInfo = alloc->make<PexDebugInfo>();
  }

  PexDebugFunctionInfo* tryFindFunctionDebugInfo(const PexObject* object,
                                                 const PexState* state,
                                                 const PexFunction* function,
                                                 const std::string& propertyName,
                                                 PexDebugFunctionType functionType);
  const PexDebugFunctionInfo* tryFindFunctionDebugInfo(const PexObject* object,
                                                       const PexState* state,
                                                       const PexFunction* function,
                                                       const std::string& propertyName,
                                                       PexDebugFunctionType functionType) const;
  PexString getString(const identifier_ref& str);
  identifier_ref getStringValue(const PexString& str) const;
  PexUserFlags getUserFlag(PexString name, uint8_t bitNum);
  size_t getUserFlagCount() const noexcept;

  static PexFile* read(allocators::ChainedPool* alloc, PexReader& rdr);
  void write(PexWriter& wtr) const;
  void writeAsm(PexAsmWriter& wtr) const;

private:
  friend allocators::ChainedPool;

  ~PexFile();

  allocators::ReffyStringPool* stringTable;

  std::vector<std::pair<PexString, uint8_t>> userFlagTable;
  std::unordered_map<size_t, size_t> userFlagTableLookup;
};

}}
