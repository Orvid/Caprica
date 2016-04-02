#pragma once

#include <common/CapricaConfig.h>
#include <common/CapricaError.h>

namespace caprica {

struct EngineLimits final
{
  enum class Type {
    PexObject_NameLength,
    PexObject_PropertyCount,
    PexObject_StateCount,
    PexObject_VariableCount,
  };

  static void checkLimit(const CapricaFileLocation& location, Type tp, size_t value) {
    const auto becauseLong = [](size_t value, size_t max) {
      return max != 0 && value > max;
    };

    switch (tp) {
      case Type::PexObject_NameLength:
        if (becauseLong(value, CapricaConfig::EngineLimits::maxObjectNameLength))
          CapricaError::Warning::W2001_EngineLimits_PexObject_Name(location, value, CapricaConfig::EngineLimits::maxObjectNameLength);
        break;
      case Type::PexObject_PropertyCount:
        if (becauseLong(value, CapricaConfig::EngineLimits::maxPropertiesPerObject))
          CapricaError::Warning::W2002_EngineLimits_PexObject_PropertyCount(location, CapricaConfig::EngineLimits::maxPropertiesPerObject, value);
        break;
      case Type::PexObject_StateCount:
        if (becauseLong(value, CapricaConfig::EngineLimits::maxStatesPerObject))
          CapricaError::Warning::W2003_EngineLimits_PexObject_StateCount(location, CapricaConfig::EngineLimits::maxStatesPerObject, value);
        break;
      case Type::PexObject_VariableCount:
        if (becauseLong(value, CapricaConfig::EngineLimits::maxVariablesPerObject))
          CapricaError::Warning::W2004_EngineLimits_PexObject_VariableCount(location, CapricaConfig::EngineLimits::maxVariablesPerObject, value);
        break;
    }
  }
};

}
