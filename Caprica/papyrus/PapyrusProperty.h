#pragma once

#include <string>

#include <papyrus/PapyrusFunction.h>
#include <papyrus/PapyrusType.h>
#include <papyrus/PapyrusUserFlags.h>
#include <papyrus/PapyrusValue.h>

namespace caprica { namespace papyrus {

struct PapyrusProperty final
{
  std::string name{ "" };
  std::string documentationComment{ "" };
  PapyrusType type{ };
  PapyrusUserFlags userFlags{ PapyrusUserFlags::None };
  PapyrusFunction* readFunction{ nullptr };
  PapyrusFunction* writeFunction{ nullptr };
  PapyrusValue defaultValue{ };

  PapyrusProperty() = default;
  ~PapyrusProperty() {
    if (readFunction)
      delete readFunction;
    if (writeFunction)
      delete writeFunction;
  }
};

}}
