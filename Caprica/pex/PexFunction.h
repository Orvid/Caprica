#pragma once

#include <string>

#include <common/IntrusiveLinkedList.h>

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
  IntrusiveLinkedList<PexFunctionParameter> parameters{ };
  IntrusiveLinkedList<PexLocalVariable> locals{ };
  IntrusiveLinkedList<PexInstruction> instructions{ };

  explicit PexFunction() = default;
  PexFunction(const PexFunction&) = delete;
  ~PexFunction() = default;

  static PexFunction *read(allocators::ChainedPool *alloc, PexReader &rdr, bool isProperty, GameID gameType);
  void write(PexWriter& wtr) const;
  void writeAsm(const PexFile* file, const PexObject* obj, const PexState* state, PexDebugFunctionType funcType, std::string propName, PexAsmWriter& wtr) const;

private:
  friend IntrusiveLinkedList<PexFunction>;
  PexFunction* next{ nullptr };
};

}}
