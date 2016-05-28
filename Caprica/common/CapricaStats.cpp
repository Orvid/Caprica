#include <common/CapricaStats.h>

#include <iostream>
#include <type_traits>

namespace caprica {

CapricaStats::counter_type CapricaStats::peekedTokenCount{ 0 };
CapricaStats::counter_type CapricaStats::consumedTokenCount{ 0 };
CapricaStats::counter_type CapricaStats::lexedFilesCount{ 0 };
CapricaStats::counter_type CapricaStats::inputFileCount{ 0 };

template<typename CounterType, typename NopType>
static std::enable_if_t<!std::is_same<CounterType, NopType>::value> internalOutputStats() {
  using s = CapricaStats;
  using counter_type = CounterType;
  const auto perc = [](const counter_type& a, const counter_type& b) -> double {
    return ((double)a / (double)b) * 100;
  };
  const auto tim = [](const counter_type& a, const counter_type& b) -> double {
    return (double)a / (double)b;
  };
  std::cout << "Lexed " << s::consumedTokenCount << " tokens of which " << s::peekedTokenCount << " were peeked. (" << perc(s::peekedTokenCount, s::consumedTokenCount) << "%)" << std::endl;
  std::cout << "Lexed " << s::lexedFilesCount << " files so " << (s::lexedFilesCount - s::inputFileCount) << " were lexed twice. Each file was lexed " << tim(s::lexedFilesCount, s::inputFileCount) << " times on average." << std::endl;
}

template<typename CounterType, typename NopType>
static typename std::enable_if_t<std::is_same<CounterType, NopType>::value> internalOutputStats() {
}

void CapricaStats::outputStats() {
  internalOutputStats<decltype(CapricaStats::peekedTokenCount), CapricaStats::NopIncStruct>();
}

}
