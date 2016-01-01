#pragma once

#include <papyrus/PapyrusObject.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/parser/PapyrusFileLocation.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusParentExpression final : public PapyrusExpression
{
  // We do this this way because we can't
  // get the type in resultType() otherwise.
  PapyrusType type{ };

  PapyrusParentExpression(parser::PapyrusFileLocation loc) : PapyrusExpression(loc) { }
  virtual ~PapyrusParentExpression() override = default;

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    return pex::PexValue::Identifier(file->getString("parent"));
  }

  virtual void semantic(PapyrusResolutionContext* ctx) override {
    type = ctx->resolveType(type);
    if (ctx->object->parentClass != type)
      ctx->fatalError("An error occured while resolving the parent type!");
    if (ctx->object->parentClass == PapyrusType::None())
      ctx->fatalError("Parent is invalid in a script with no parent!");
  }

  virtual PapyrusType resultType() const override {
    return type;
  }
};

}}}
