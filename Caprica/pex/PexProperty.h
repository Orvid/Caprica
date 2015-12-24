#pragma once

#include <pex/PexFunction.h>
#include <pex/PexString.h>
#include <pex/PexUserFlags.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexProperty final
{
  PexString name{ };
  PexString typeName{ };
  PexString documentationString{ };
  PexUserFlags userFlags{ };
  PexString autoVar{ };
  PexFunction* readFunction{ nullptr };
  PexFunction* writeFunction{ nullptr };
  bool isAutoReadOnly{ false };

  PexProperty() = default;
  ~PexProperty() {
    if (readFunction)
      delete readFunction;
    if (writeFunction)
      delete writeFunction;
  }

  void write(PexWriter& wtr) const;
};

}}
