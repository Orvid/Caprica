#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusIdentifier.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>
#include <papyrus/parser/PapyrusFileLocation.h>

#include <pex/PexFile.h>
#include <pex/PexFunctionBuilder.h>
#include <pex/PexValue.h>

namespace caprica { namespace papyrus { namespace expressions {

struct PapyrusFunctionCallExpression final : public PapyrusExpression
{
  struct Parameter final
  {
    std::string name{ "" };
    PapyrusExpression* value{ nullptr };

    Parameter() = default;
    ~Parameter() {
      if (value)
        delete value;
    }
  };
  PapyrusIdentifier function{ };
  std::vector<Parameter*> arguments{ };

  PapyrusFunctionCallExpression(parser::PapyrusFileLocation loc) : PapyrusExpression(loc) { }
  virtual ~PapyrusFunctionCallExpression() override {
    for (auto a : arguments)
      delete a;
  }

  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    return generateLoad(file, bldr, nullptr);
  }

  pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr, PapyrusExpression* base) const;
  virtual void semantic(PapyrusResolutionContext* ctx) override;
  virtual PapyrusType resultType() const override;
};

}}}
