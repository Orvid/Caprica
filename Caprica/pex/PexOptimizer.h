#pragma once

#include <pex/PexFile.h>

namespace caprica { namespace pex {
struct PexOptimizer final
{
  static void optimize(PexFile* file) {
    PexOptimizer opt{ };
    for (auto o : file->objects)
      opt.optimize(file, o);
  }

private:
  PexOptimizer() = default;
  ~PexOptimizer() = default;

  void optimize(PexFile* file, PexObject* object) {
    for (auto s : object->states)
      optimize(file, object, s);
  }

  void optimize(PexFile* file, PexObject* object, PexState* state) {
    for (auto f : state->functions)
      optimize(file, object, state, f, "", PexDebugFunctionType::Normal);
  }

  void optimize(PexFile* file,
                PexObject* object,
                PexState* state,
                PexFunction* function,
                const std::string& propertyName,
                PexDebugFunctionType functionType);
};
}}
