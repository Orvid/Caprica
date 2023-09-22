#pragma once

#include <atomic>

namespace caprica {

struct CapricaStats final {
private:
  struct NopIncStruct final
  {
    size_t val;

    NopIncStruct& operator++(int) { val++; return *this; }
    NopIncStruct& operator=(size_t f) { val = f; return *this; }
  };


  //using counter_type = std::atomic<size_t>;
  //using counter_type = size_t;
  using counter_type = NopIncStruct;

public:
  static counter_type peekedTokenCount;
  static counter_type consumedTokenCount;
  static counter_type importedFileCount;
  static counter_type inputFileCount;
  static counter_type lexedFilesCount;
  static counter_type allocatedHeapCount;
  static counter_type freedHeapCount;

  static void outputStats();
  static void outputImportedCount();

};

}
