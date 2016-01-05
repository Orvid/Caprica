#pragma once

#include <vector>

#include <pex/PexAsmWriter.h>
#include <pex/PexFunction.h>
#include <pex/PexString.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexFile;
struct PexObject;

struct PexState final
{
  PexString name{ };
  std::vector<PexFunction*> functions{ };

  PexState() = default;
  ~PexState() {
    for (auto f : functions)
      delete f;
  }

  void write(PexWriter& wtr) const;
  void writeAsm(const PexFile* file, const PexObject* obj, PexAsmWriter& wtr) const;
};

}}
