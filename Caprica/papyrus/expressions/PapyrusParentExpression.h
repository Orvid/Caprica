#pragma once

#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/PapyrusObject.h>
#include <papyrus/PapyrusType.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusParentExpression final : public PapyrusExpression {
  // We do this this way because we can't
  // get the type in resultType() otherwise.
  PapyrusType type;

  explicit PapyrusParentExpression(CapricaFileLocation loc, const PapyrusType& tp)
      : PapyrusExpression(loc), type(tp) { }
  PapyrusParentExpression(const PapyrusParentExpression&) = delete;
  virtual ~PapyrusParentExpression() override = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder&) const override {
    return pex::PexValue::Identifier(file->getString("parent"));
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    type = ctx->resolveType(type);
    if (ctx->object->parentClass != type)
      ctx->reportingContext.fatal(location, "An error occured while resolving the parent type!");
    if (ctx->object->parentClass.type == PapyrusType::Kind::None)
      ctx->reportingContext.fatal(location, "Parent is invalid in a script with no parent!");
  }

  virtual PapyrusType resultType() const override { return type; }

  virtual PapyrusParentExpression* asParentExpression() override { return this; }
};

}}}
