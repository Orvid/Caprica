#pragma once

#include <cstdint>
#include <ctime>
#include <map>
#include <string>
#include <vector>

#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct StringIdx final
{
  size_t index;
};

struct PexFile final
{
  uint8_t majorVersion{ 3 };
  uint8_t minorVersion{ 9 };
  uint16_t gameID{ 2 }; // Default to Fallout 4
  time_t compilationTime{ };
  std::string sourceFileName{ "" };
  std::string userName{ "" };
  std::string computerName{ "" };
  PexDebugInfo* debugInfo{ nullptr };

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
