#pragma once

#include <common/CapricaConfig.h>
#include <common/CapricaReportingContext.h>
#include <common/identifier_ref.h>

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
    PexObject_StaticFunctionCount,
    PexObject_VariableCount,
    PexState_FunctionCount,
    PexObject_GuardCount,
  };

  static void checkLimit(CapricaReportingContext& repCtx, CapricaFileLocation location, Type tp, size_t value, identifier_ref referenceName = "") {
    const auto exceedsLimit = [](size_t value, size_t max) {
      return max != 0 && value > max;
    };

    switch (tp) {
      case Type::ArrayLength:
        if (exceedsLimit(value, conf::EngineLimits::maxArrayLength))
          repCtx.warning_W2001_EngineLimits_ArrayLength(location, value, conf::EngineLimits::maxArrayLength);
        break;
      case Type::PexFile_UserFlagCount:
        if (exceedsLimit(value, conf::EngineLimits::maxUserFlags))
          repCtx.warning_W2002_EngineLimits_PexFile_UserFlagCount(location, value, conf::EngineLimits::maxUserFlags);
        break;
      case Type::PexFunction_ParameterCount:
        if (exceedsLimit(value, conf::EngineLimits::maxParametersPerFunction))
          repCtx.warning_W2003_EngineLimits_PexFunction_ParameterCount(location, value, referenceName.to_string().c_str(), conf::EngineLimits::maxParametersPerFunction);
        break;
      case Type::PexObject_EmptyStateFunctionCount:
        if (exceedsLimit(value, conf::EngineLimits::maxFunctionsInEmptyStatePerObject))
          repCtx.warning_W2004_EngineLimits_PexObject_EmptyStateFunctionCount(location, value, conf::EngineLimits::maxFunctionsInEmptyStatePerObject);
        break;
      case Type::PexObject_InitialValueCount:
        if (exceedsLimit(value, conf::EngineLimits::maxInitialValuesPerObject))
          repCtx.warning_W2005_EngineLimits_PexObject_InitialValueCount(location, value, conf::EngineLimits::maxInitialValuesPerObject);
        break;
      case Type::PexObject_NamedStateCount:
        if (exceedsLimit(value, conf::EngineLimits::maxNamedStatesPerObject))
          repCtx.warning_W2006_EngineLimits_PexObject_NamedStateCount(location, value, conf::EngineLimits::maxNamedStatesPerObject);
        break;
      case Type::PexObject_PropertyCount:
        if (exceedsLimit(value, conf::EngineLimits::maxPropertiesPerObject))
          repCtx.warning_W2007_EngineLimits_PexObject_PropertyCount(location, value, conf::EngineLimits::maxVariablesPerObject);
        break;
      case Type::PexObject_StaticFunctionCount:
        if (exceedsLimit(value, conf::EngineLimits::maxStaticFunctionsPerObject))
          repCtx.warning_W2008_EngineLimits_PexObject_StaticFunctionCount(location, value, conf::EngineLimits::maxStaticFunctionsPerObject);
        break;
      case Type::PexObject_VariableCount:
        if (exceedsLimit(value, conf::EngineLimits::maxVariablesPerObject))
          repCtx.warning_W2009_EngineLimits_PexObject_VariableCount(location, value, conf::EngineLimits::maxVariablesPerObject);
        break;
      case Type::PexState_FunctionCount:
        if (exceedsLimit(value, conf::EngineLimits::maxFunctionsPerState))
          repCtx.warning_W2010_EngineLimits_PexState_FunctionCount(location, value, referenceName.to_string().c_str(), conf::EngineLimits::maxFunctionsPerState);
        break;
      case Type::PexObject_GuardCount:
        if (exceedsLimit(value, conf::EngineLimits::maxGuardsPerObject))
          repCtx.warning_W2011_EngineLimits_PexObject_GuardCount(location, value, conf::EngineLimits::maxGuardsPerObject);
        break;

    }
  }
};

}
