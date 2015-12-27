#pragma once

namespace caprica { namespace papyrus { struct PapyrusResolutionContext; } }

#include <papyrus/PapyrusFunction.h>
#include <papyrus/PapyrusType.h>

namespace caprica { namespace papyrus {

struct PapyrusFunction;

struct PapyrusResolutionContext final
{
  const PapyrusFunction* function{ nullptr };

  PapyrusType resolveType(PapyrusType tp) {
    return tp;
  }
};

}}
