#pragma once

#include <string>

#include <papyrus/PapyrusScript.h>

namespace caprica { namespace pex {

struct PexReflector final
{
  static papyrus::PapyrusScript* reflectScript(const std::string& filename);
};

}}