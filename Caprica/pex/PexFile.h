#pragma once

#include <cstdint>
#include <ctime>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <pex/PexDebugInfo.h>
#include <pex/PexObject.h>
#include <pex/PexString.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

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
  std::vector<PexObject*> objects{ };

  PexFile() = default;
  ~PexFile() {
    if (debugInfo)
      delete debugInfo;
    for (auto o : objects)
      delete o;
  }

  PexString getString(std::string str) {
    auto a = stringTableLookup.find(str);
    if (a != stringTableLookup.end())
    {
      auto ret = PexString();
      ret.index = a->second;
      return ret;
    }
    stringTable.push_back(str);
    stringTableLookup[str] = stringTable.size() - 1;
  }

  PexUserFlag getUserFlag(PexString name, uint8_t bitNum) {
    auto a = userFlagTableLookup.find(name);
    if (a != userFlagTableLookup.end()) {
      auto flag = PexUserFlag();
      flag.data = 1ULL << a->second;
      return flag;
    }
    userFlagTable.push_back(std::make_pair(name, bitNum));
    userFlagTableLookup[name] = bitNum;
  }

  void write(PexWriter& wtr) const;

private:
  std::vector<std::string> stringTable;
  std::map<std::string, size_t> stringTableLookup;

  std::vector<std::pair<PexString, uint8_t>> userFlagTable;
  std::map<PexString, size_t> userFlagTableLookup;
};

}}
