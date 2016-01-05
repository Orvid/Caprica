#pragma once

#include <pex/PexAsmWriter.h>
#include <pex/PexString.h>

namespace caprica { namespace pex {

struct PexFile;
struct PexReader;
struct PexWriter;

struct PexLocalVariable final
{
  PexString name{ };
  PexString type{ };

  PexLocalVariable() = default;
  ~PexLocalVariable() = default;

  static PexLocalVariable* read(PexReader& rdr);
  void write(PexWriter& wtr) const;
  void writeAsm(const PexFile* file, PexAsmWriter& wtr) const;
};

}}
