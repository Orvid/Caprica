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

#include <common/GameID.h>
#include <pex/PexAsmWriter.h>
#include <pex/PexDebugInfo.h>
#include <pex/PexObject.h>
#include <pex/PexReader.h>
#include <pex/PexString.h>
#include <pex/PexWriter.h>

namespace caprica {

namespace papyrus {
struct PapyrusScript;
}

namespace pex {

constexpr uint32_t PEX_MAGIC_NUM = 0xFA57C0DE;
constexpr uint32_t PEX_MAGIC_NUM_BE = 0xDE57C0FA;

struct PexFile final {
  allocators::ChainedPool* alloc;

  uint8_t majorVersion { 3 };
  uint8_t minorVersion { 12 };
  GameID gameID { GameID::Starfield }; // Default to Starfield
  time_t compilationTime {};
  std::string_view sourceFileName { "" };
  std::string userName { "" };
  std::string computerName { "" };
  PexDebugInfo* debugInfo { nullptr };
  IntrusiveLinkedList<PexObject> objects {};

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

  void setGameAndVersion(GameID game) {
    gameID = game;
    switch (game) {
      case GameID::Skyrim:
        majorVersion = 3;
        minorVersion = 2; // Vanilla base game uses 3.1, vanilla expansions use 3.2, all SE and AE scripts use 3.2
        break;
      case GameID::Fallout4:
        majorVersion = 3;
        minorVersion = 9; // apparently doesn't change for fallout 4?
        break;
      case GameID::Fallout76:
        majorVersion = 3;
        minorVersion = 15; // TODO: Verify why fallout76 version is > starfield version??
        break;
      case GameID::Starfield:
        majorVersion = 3;
        minorVersion = 12;
        break;
      default:
        majorVersion = 0;
        minorVersion = 0;
        break;
    }
  }
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

}
}
