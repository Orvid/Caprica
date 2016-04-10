#pragma once

#include <cstdint>
#include <ctime>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <pex/PexAsmWriter.h>
#include <pex/PexDebugInfo.h>
#include <pex/PexObject.h>
#include <pex/PexReader.h>
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

  explicit PexFile() = default;
  PexFile(const PexFile&) = delete;
  ~PexFile() {
    if (debugInfo)
      delete debugInfo;
    for (auto o : objects)
      delete o;
  }

  void ensureDebugInfo() {
    if (!debugInfo)
      debugInfo = new PexDebugInfo();
  }

  PexDebugFunctionInfo* tryFindFunctionDebugInfo(const PexObject* object,
                                                 const PexState* state,
                                                 const PexFunction* function,
                                                 const std::string& propertyName,
                                                 PexDebugFunctionType functionType) {
    if (debugInfo) {
      assert(function);
      assert(object);
      auto fName = propertyName == "" ? getStringValue(function->name) : propertyName;
      auto objectName = getStringValue(object->name);
      auto stateName = state ? getStringValue(state->name) : "";

      for (auto fi : debugInfo->functions) {
        if (getStringValue(fi->objectName) == objectName &&
            getStringValue(fi->stateName) == stateName &&
            getStringValue(fi->functionName) == fName &&
            fi->functionType == functionType) {
          return fi;
        }
      }
    }
    return nullptr;
  }

  const PexDebugFunctionInfo* tryFindFunctionDebugInfo(const PexObject* object,
                                                 const PexState* state,
                                                 const PexFunction* function,
                                                 const std::string& propertyName,
                                                 PexDebugFunctionType functionType) const {
    return ((PexFile*)this)->tryFindFunctionDebugInfo(object, state, function, propertyName, functionType);
  }

  PexString getString(const std::string& str) {
    auto a = stringTableLookup.find(str);
    if (a != stringTableLookup.end())
    {
      auto ret = PexString();
      ret.index = a->second;
      return ret;
    }
    stringTable.push_back(str);
    stringTableLookup[str] = stringTable.size() - 1;
    auto ret = PexString();
    ret.index = stringTable.size() - 1;
    return ret;
  }

  std::string getStringValue(const PexString& str) const {
    return stringTable[str.index];
  }

  PexUserFlags getUserFlag(PexString name, uint8_t bitNum) {
    auto a = userFlagTableLookup.find(name);
    if (a != userFlagTableLookup.end()) {
      auto flag = PexUserFlags();
      flag.data = 1ULL << a->second;
      return flag;
    }
    userFlagTable.push_back(std::make_pair(name, bitNum));
    userFlagTableLookup[name] = bitNum;
    auto flag = PexUserFlags();
    flag.data = 1ULL << bitNum;
    return flag;
  }

  size_t getUserFlagCount() {
    return userFlagTable.size();
  }

  static PexFile* read(PexReader& rdr);
  void write(PexWriter& wtr) const;
  void writeAsm(PexAsmWriter& wtr) const;

private:
  std::vector<std::string> stringTable;
  std::map<std::string, size_t> stringTableLookup;

  std::vector<std::pair<PexString, uint8_t>> userFlagTable;
  std::map<PexString, size_t> userFlagTableLookup;
};

}}
