#pragma once

#include <string>
#include <vector>

#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexDebugInfo final
{


  PexFile() = default;
  ~PexFile() {
    if (debugInfo)
      delete debugInfo;
  }

  StringIdx getString(std::string str) {
    auto a = stringTableLookup.find(str);
    if (a != stringTableLookup.end())
      return StringIdx{ a->second };
    stringTable.push_back(str);
    stringTableLookup[str] = stringTable.size() - 1;
  }

  void writeToFile(PexWriter& wtr);

private:
  std::vector<std::string> stringTable;
  std::map<std::string, size_t> stringTableLookup;
};

}}
