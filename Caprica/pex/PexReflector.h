#pragma once

#include <string>

#include <papyrus/PapyrusScript.h>
#include <pex/PexFile.h>

namespace caprica { namespace pex {

struct PexReflector final {
  static papyrus::PapyrusScript* reflectScript(PexFile* pex);
};

}}
