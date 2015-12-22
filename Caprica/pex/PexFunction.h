#pragma once

#include <vector>

#include <pex/PexString.h>
#include <pex/PexUserFlags.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexFunction final
{
  // If name is invalid, it is assumed this is
  // a property function, and the name won't be
  // written to file.
  PexString name{ };
  PexString returnTypeName{ };
  PexString documenationString{ };
  PexUserFlags userFlags{ };
  bool isNative{ false };
  bool isGlobal{ false };

  void write(PexWriter& wtr) const;
};

}}
