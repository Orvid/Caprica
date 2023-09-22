#pragma once

#include <common/CapricaFileLocation.h>
#include <common/identifier_ref.h>
#include <common/IntrusiveLinkedList.h>

#include <papyrus/PapyrusIdentifier.h>
#include <papyrus/PapyrusResolutionContext.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusValue.h>

#include <pex/PexFile.h>
#include <pex/PexFunction.h>
#include <pex/PexFunctionParameter.h>
#include <pex/PexObject.h>
#include <pex/PexState.h>

namespace caprica { namespace papyrus {

struct PapyrusFunctionParameter final {
  identifier_ref name { "" };
  const size_t index;
  PapyrusType type;
  PapyrusValue defaultValue { PapyrusValue::Default() };

  const CapricaFileLocation location;

  explicit PapyrusFunctionParameter(CapricaFileLocation loc, size_t idx, PapyrusType&& tp)
      : location(loc), index(idx), type(std::move(tp)) { }
  PapyrusFunctionParameter(const PapyrusFunctionParameter&) = delete;
  ~PapyrusFunctionParameter() = default;

  void buildPex(pex::PexFile* file, pex::PexObject*, pex::PexFunction* func) const {
    auto param = file->alloc->make<pex::PexFunctionParameter>();
    param->name = file->getString(name);
    param->type = type.buildPex(file);
    func->parameters.push_back(param);
  }

  void semantic(PapyrusResolutionContext* ctx) {
    type = ctx->resolveType(type, true);
    defaultValue = ctx->coerceDefaultValue(defaultValue, type);
  }

private:
  friend IntrusiveLinkedList<PapyrusFunctionParameter>;
  template <typename T>
  friend struct IntrusiveLinkedList;
  template <typename T>
  template <typename T2>
  friend struct IntrusiveLinkedList<T>::LockstepIterator;
  PapyrusFunctionParameter* next { nullptr };
};

}}
