#pragma once

#include <vector>

#include <pex/PexFunctionParameter.h>
#include <pex/PexInstruction.h>
#include <pex/PexLocalVariable.h>
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
  std::vector<PexFunctionParameter*> parameters{ };
  std::vector<PexLocalVariable*> locals{ };
  std::vector<PexInstruction*> instructions{ };

  PexFunction() = default;
  ~PexFunction() {
    for (auto p : parameters)
      delete p;
    for (auto l : locals)
      delete l;
    for (auto i : instructions)
      delete i;
  }

  void write(PexWriter& wtr) const;
};

}}
