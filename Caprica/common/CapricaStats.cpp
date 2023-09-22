#include <common/CapricaStats.h>

#include <iostream>
#include <type_traits>

namespace caprica {

CapricaStats::counter_type CapricaStats::peekedTokenCount { 0 };
CapricaStats::counter_type CapricaStats::consumedTokenCount { 0 };
CapricaStats::counter_type CapricaStats::lexedFilesCount { 0 };
CapricaStats::counter_type CapricaStats::importedFileCount { 0 };
CapricaStats::counter_type CapricaStats::inputFileCount { 0 };
CapricaStats::counter_type CapricaStats::allocatedHeapCount { 0 };
CapricaStats::counter_type CapricaStats::freedHeapCount { 0 };

template <typename CounterType, typename NopType>
static std::enable_if_t<!std::is_same<CounterType, NopType>::value> internalOutputStats() {
  // This forces the print lines below to be delay typed, so that they only try to print
  // when the counters are set to a type that actually counts.
  using s = typename std::enable_if<!std::is_same<CounterType, NopType>::value, CapricaStats>::type;
  using counter_type = CounterType;
  const auto perc = [](const counter_type& a, const counter_type& b) -> double {
    return ((double)a / (double)b) * 100;
  };
  const auto tim = [](const counter_type& a, const counter_type& b) -> double {
    return (double)a / (double)b;
  };
  std::cout << "Lexed " << s::consumedTokenCount << " tokens of which " << s::peekedTokenCount << " were peeked. ("
            << perc(s::peekedTokenCount, s::consumedTokenCount) << "%)" << std::endl;
  std::cout << "Lexed " << s::lexedFilesCount << " files so " << (s::lexedFilesCount - s::inputFileCount)
            << " were lexed twice. Each file was lexed " << tim(s::lexedFilesCount, s::inputFileCount)
            << " times on average." << std::endl;
  std::cout << "Allocated " << s::allocatedHeapCount << " heaps and freed " << s::freedHeapCount << " heaps."
            << std::endl;
}

template <typename CounterType, typename NopType>
static typename std::enable_if_t<std::is_same<CounterType, NopType>::value> internalOutputStats() {
}

void CapricaStats::outputStats() {
  internalOutputStats<decltype(CapricaStats::peekedTokenCount), CapricaStats::NopIncStruct>();
}

void CapricaStats::outputImportedCount() {
  std::cout << "Imported " << importedFileCount.val << " files." << std::endl;
}

}
