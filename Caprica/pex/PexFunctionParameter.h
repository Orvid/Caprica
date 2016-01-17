#pragma once

#include <pex/PexAsmWriter.h>
#include <pex/PexReader.h>
#include <pex/PexString.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexFile;

struct PexFunctionParameter final
{
  PexString name{ };
  PexString type{ };

  explicit PexFunctionParameter() = default;
  ~PexFunctionParameter() = default;

  static PexFunctionParameter* read(PexReader& rdr);
  void write(PexWriter& wtr) const;
  void writeAsm(const PexFile* file, PexAsmWriter& wtr) const;
};

}}
