#include <common/CapricaStats.h>

#include <iostream>

namespace caprica {

CapricaStats::counter_type CapricaStats::peekedTokenCount{ 0 };
CapricaStats::counter_type CapricaStats::consumedTokenCount{ 0 };
CapricaStats::counter_type CapricaStats::lexedFilesCount{ 0 };
CapricaStats::counter_type CapricaStats::inputFileCount{ 0 };

void CapricaStats::outputStats() {
#if 1
  const auto perc = [](const counter_type& a, const counter_type& b) -> double {
    return ((double)a / (double)b) * 100;
  };
  const auto tim = [](const counter_type& a, const counter_type& b) -> double {
    return (double)a / (double)b;
  };
  std::cout << "Lexed " << consumedTokenCount << " tokens of which " << peekedTokenCount << " were peeked. (" << perc(peekedTokenCount, consumedTokenCount) << "%)" << std::endl;
  std::cout << "Lexed " << lexedFilesCount << " files so " << (lexedFilesCount - inputFileCount) << " were lexed twice. Each file was lexed " << tim(lexedFilesCount, inputFileCount) << " times on average." << std::endl;
#endif
}

}
