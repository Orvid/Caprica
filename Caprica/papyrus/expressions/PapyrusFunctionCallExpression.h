#pragma once

#include <string>
#include <vector>

#include <papyrus/PapyrusIdentifier.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/expressions/PapyrusExpression.h>

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

    explicit Parameter() = default;
    Parameter(const Parameter&) = delete;
    ~Parameter() {
      if (value)
        delete value;
    }
  };
  PapyrusIdentifier function;
  std::vector<Parameter*> arguments{ };

  explicit PapyrusFunctionCallExpression(CapricaFileLocation loc, const PapyrusIdentifier& f) : PapyrusExpression(loc), function(f) { }
  PapyrusFunctionCallExpression(const PapyrusFunctionCallExpression&) = delete;
  virtual ~PapyrusFunctionCallExpression() override {
    for (auto a : arguments)
      delete a;
  }

  pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr, PapyrusExpression* base) const;
  virtual pex::PexValue generateLoad(pex::PexFile* file, pex::PexFunctionBuilder& bldr) const override {
    return generateLoad(file, bldr, nullptr);
  }

  void semantic(PapyrusResolutionContext* ctx, PapyrusExpression* baseExpression);
  virtual void semantic(PapyrusResolutionContext* ctx) override {
    semantic(ctx, nullptr);
  }

  virtual PapyrusType resultType() const override;

private:
  bool isPoisonedReturn{ false };
};

}}}
