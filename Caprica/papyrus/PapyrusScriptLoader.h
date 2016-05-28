#pragma once

#include <string>

#include <papyrus/PapyrusScript.h>

namespace caprica { namespace papyrus {

struct PapyrusScriptLoader final
{
  enum class LoadType {
    Unknown,
    Reference,
    CheckOnly,
    Compile,
  };

  static PapyrusScript* loadScript(const std::string& reportedName,
                                   const std::string& fullPath,
                                   const std::string& baseOutputDirectory,
                                   LoadType loadType);
};

}}
