#pragma once

#include <atomic>

namespace caprica {

struct CapricaStats final {
private:
  struct NopIncStruct final {
    size_t val;

    NopIncStruct& operator++(int) { return *this; }
    NopIncStruct& operator=(size_t) { return *this; }
  };

  // using counter_type = std::atomic<size_t>;
  using counter_type = size_t;
  // using counter_type = NopIncStruct;

public:
  static NopIncStruct peekedTokenCount;
  static NopIncStruct consumedTokenCount;
  static counter_type importedFileCount;
  static counter_type inputFileCount;
  static NopIncStruct lexedFilesCount;
  static NopIncStruct allocatedHeapCount;
  static NopIncStruct freedHeapCount;

  static void outputStats();
  static void outputImportedCount();
};

}
