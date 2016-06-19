#pragma once

#include <string>
#include <vector>

#include <common/allocators/ChainedDestructedPooledAllocator.h>

#include <pex/PexAsmWriter.h>
#include <pex/PexDebugFunctionInfo.h>
#include <pex/PexFunctionParameter.h>
#include <pex/PexInstruction.h>
#include <pex/PexLocalVariable.h>
#include <pex/PexReader.h>
#include <pex/PexString.h>
#include <pex/PexUserFlags.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

struct PexFile;
struct PexObject;
struct PexState;

using PexInstructionList = allocators::ChainedDestructedPooledAllocator<PexInstruction, 128>;

struct PexFunction final
{
  // If name is invalid, it is assumed this is
  // a property function, and the name won't be
  // written to file.
  PexString name{ };
  PexString returnTypeName{ };
  PexString documentationString{ };
  PexUserFlags userFlags{ };
  bool isNative{ false };
  bool isGlobal{ false };
  std::vector<PexFunctionParameter*> parameters{ };
  std::vector<PexLocalVariable*> locals{ };
  PexInstructionList instructions{ };

  explicit PexFunction() = default;
  PexFunction(const PexFunction&) = delete;
  ~PexFunction() {
    for (auto p : parameters)
      delete p;
    for (auto l : locals)
      delete l;
  }

  static PexFunction* read(PexReader& rdr, bool isProperty);
  void write(PexWriter& wtr) const;
  void writeAsm(const PexFile* file, const PexObject* obj, const PexState* state, PexDebugFunctionType funcType, std::string propName, PexAsmWriter& wtr) const;
};

}}
