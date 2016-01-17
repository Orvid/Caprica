#pragma once

#include <cstdint>
#include <vector>

#include <pex/PexAsmWriter.h>
#include <pex/PexProperty.h>
#include <pex/PexReader.h>
#include <pex/PexState.h>
#include <pex/PexString.h>
#include <pex/PexStruct.h>
#include <pex/PexUserFlags.h>
#include <pex/PexVariable.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexFile;

struct PexObject final
{
  PexString name{ };
  PexString parentClassName{ };
  PexString documentationString{ };
  bool isConst{ false };
  PexUserFlags userFlags{ };
  PexString autoStateName{ };
  std::vector<PexStruct*> structs{ };
  std::vector<PexVariable*> variables{ };
  std::vector<PexProperty*> properties{ };
  std::vector<PexState*> states{ };

  explicit PexObject() = default;
  PexObject(const PexObject&) = delete;
  ~PexObject() {
    for (auto s : structs)
      delete s;
    for (auto v : variables)
      delete v;
    for (auto p : properties)
      delete p;
    for (auto s : states)
      delete s;
  }

  static PexObject* read(PexReader& rdr);
  void write(PexWriter& wtr) const;
  void writeAsm(const PexFile* file, PexAsmWriter& wtr) const;
};

}}
