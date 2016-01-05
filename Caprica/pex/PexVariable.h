#pragma once

#include <pex/PexAsmWriter.h>
#include <pex/PexReader.h>
#include <pex/PexString.h>
#include <pex/PexUserFlags.h>
#include <pex/PexValue.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexFile;

struct PexVariable final
{
  PexString name{ };
  PexString typeName{ };
  PexUserFlags userFlags{ };
  PexValue defaultValue{ };
  bool isConst{ false };

  PexVariable() = default;
  ~PexVariable() = default;

  static PexVariable* read(PexReader& rdr);
  void write(PexWriter& wtr) const;
  void writeAsm(const PexFile* file, PexAsmWriter& wtr) const;
};

}}
