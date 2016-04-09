#pragma once

#include <common/CapricaConfig.h>
#include <common/CapricaError.h>

namespace caprica {

struct EngineLimits final
{
  enum class Type {
    ArrayLength,
    PexFile_UserFlagCount,
    PexFunction_ParameterCount,
    PexObject_EmptyStateFunctionCount,
    PexObject_InitialValueCount,
    PexObject_NamedStateCount,
    PexObject_PropertyCount,
    PexObject_VariableCount,
    PexState_FunctionCount,
  };

  static void checkLimit(const CapricaFileLocation& location, Type tp, size_t value, const char* referenceName = nullptr) {
    const auto exceedsLimit = [](size_t value, size_t max) {
      return max != 0 && value > max;
    };

    switch (tp) {
      case Type::ArrayLength:
        if (exceedsLimit(value, CapricaConfig::EngineLimits::maxArrayLength))
          CapricaError::Warning::W2001_EngineLimits_ArrayLength(location, value, CapricaConfig::EngineLimits::maxArrayLength);
        break;
      case Type::PexFile_UserFlagCount:
        if (exceedsLimit(value, CapricaConfig::EngineLimits::maxUserFlags))
          CapricaError::Warning::W2002_EngineLimits_PexFile_UserFlagCount(location, value, CapricaConfig::EngineLimits::maxUserFlags);
        break;
      case Type::PexFunction_ParameterCount:
        if (exceedsLimit(value, CapricaConfig::EngineLimits::maxParametersPerFunction))
          CapricaError::Warning::W2003_EngineLimits_PexFunction_ParameterCount(location, value, referenceName, CapricaConfig::EngineLimits::maxParametersPerFunction);
        break;
      case Type::PexObject_EmptyStateFunctionCount:
        if (exceedsLimit(value, CapricaConfig::EngineLimits::maxFunctionsInEmptyStatePerObject))
          CapricaError::Warning::W2004_EngineLimits_PexObject_EmptyStateFunctionCount(location, value, CapricaConfig::EngineLimits::maxFunctionsInEmptyStatePerObject);
        break;
      case Type::PexObject_InitialValueCount:
        if (exceedsLimit(value, CapricaConfig::EngineLimits::maxInitialValuesPerObject))
          CapricaError::Warning::W2005_EngineLimits_PexObject_InitialValueCount(location, value, CapricaConfig::EngineLimits::maxInitialValuesPerObject);
        break;
      case Type::PexObject_NamedStateCount:
        if (exceedsLimit(value, CapricaConfig::EngineLimits::maxNamedStatesPerObject))
          CapricaError::Warning::W2006_EngineLimits_PexObject_NamedStateCount(location, value, CapricaConfig::EngineLimits::maxNamedStatesPerObject);
        break;
      case Type::PexObject_PropertyCount:
        if (exceedsLimit(value, CapricaConfig::EngineLimits::maxPropertiesPerObject))
          CapricaError::Warning::W2007_EngineLimits_PexObject_PropertyCount(location, value, CapricaConfig::EngineLimits::maxVariablesPerObject);
        break;
      case Type::PexObject_VariableCount:
        if (exceedsLimit(value, CapricaConfig::EngineLimits::maxVariablesPerObject))
          CapricaError::Warning::W2008_EngineLimits_PexObject_VariableCount(location, value, CapricaConfig::EngineLimits::maxVariablesPerObject);
        break;
      case Type::PexState_FunctionCount:
        if (exceedsLimit(value, CapricaConfig::EngineLimits::maxFunctionsPerState))
          CapricaError::Warning::W2009_EngineLimits_PexState_FunctionCount(location, value, referenceName, CapricaConfig::EngineLimits::maxFunctionsPerState);
        break;
    }
  }
};

}
