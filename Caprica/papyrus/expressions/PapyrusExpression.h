#pragma once

#include <common/CapricaFileLocation.h>
#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusType.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusArrayIndexExpression;
struct PapyrusArrayLengthExpression;
struct PapyrusFunctionCallExpression;
struct PapyrusIdentifierExpression;
struct PapyrusLiteralExpression;
struct PapyrusMemberAccessExpression;
struct PapyrusParentExpression;

struct PapyrusExpression abstract
{
  const CapricaFileLocation location;

  explicit PapyrusExpression(CapricaFileLocation loc) : location(loc) { }
  PapyrusExpression(const PapyrusExpression&) = delete;
  virtual ~PapyrusExpression() = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const abstract;
  virtual void semantic(PapyrusResolutionContext* ctx) abstract;
  virtual PapyrusType resultType() const abstract;

  // These exist because dynamic_cast is slow.
  // This list only contains expressions that we actually check for.
  virtual PapyrusArrayLengthExpression* asArrayLengthExpression() { return nullptr; }
  virtual PapyrusArrayIndexExpression* asArrayIndexExpression() { return nullptr; }
  virtual PapyrusFunctionCallExpression* asFunctionCallExpression() { return nullptr; }
  virtual PapyrusIdentifierExpression* asIdentifierExpression() { return nullptr; }
  virtual PapyrusLiteralExpression* asLiteralExpression() { return nullptr; }
  virtual PapyrusMemberAccessExpression* asMemberAccessExpression() { return nullptr; }
  virtual PapyrusParentExpression* asParentExpression() { return nullptr; }
};

}}}
