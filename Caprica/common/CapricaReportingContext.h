#pragma once

#include <cstdint>

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include <common/allocators/FileOffsetPool.h>
#include <common/CapricaFileLocation.h>
#include <common/identifier_ref.h>
#include <common/UtilMacros.h>

namespace caprica {

struct CapricaReportingContext final {
  std::string filename;
  // TODO: fix Imports hack
  bool m_QuietWarnings { false };
  size_t warningCount { 0 };
  size_t errorCount { 0 };

  CapricaReportingContext() = delete;
  CapricaReportingContext(const CapricaReportingContext& other) = delete;
  CapricaReportingContext(CapricaReportingContext&& other) = delete;
  CapricaReportingContext& operator=(const CapricaReportingContext&) = delete;
  CapricaReportingContext& operator=(CapricaReportingContext&&) = delete;

  CapricaReportingContext(const std::string& name) : filename(name) { lineOffsets.push_back(0); }
  ~CapricaReportingContext() = default;

  size_t getLocationLine(CapricaFileLocation location, size_t lastLineHint = 0);
  void pushNextLineOffset(CapricaFileLocation location) { lineOffsets.push_back(location.startOffset); }

  NEVER_INLINE
  static void breakIfDebugging();
  NEVER_INLINE
  void exitIfErrors();

  template <typename... Args>
  NEVER_INLINE void error(CapricaFileLocation location, fmt::format_string<Args...> msg, Args&&... args) {
    errorCount++;
    maybePushMessage(this, &location, "Error", 0, fmt::format(msg, std::forward<Args>(args)...), true);
    breakIfDebugging();
  }

  template <typename... Args>
  [[noreturn]] NEVER_INLINE void fatal(CapricaFileLocation location, fmt::format_string<Args...> msg, Args&&... args) {
    maybePushMessage(this, &location, "Fatal Error", 0, fmt::format(msg, std::forward<Args>(args)...), true);
    throw std::runtime_error("");
  }

  // The difference between this and fatal is that this is intended for places
  // where the logic of Caprica itself has failed, and a location in a source
  // file is likely not available.
  template <typename... Args>
  [[noreturn]] NEVER_INLINE static void logicalFatal(fmt::format_string<Args...> msg, Args&&... args) {
    maybePushMessage(nullptr, nullptr, "Fatal Error", 0, fmt::format(msg, std::forward<Args>(args)...), true);
    throw std::runtime_error("");
  }

#define DEFINE_WARNING_A0(num, id, msg) \
  NEVER_INLINE void warning_W##num##_##id(CapricaFileLocation location) { warning(location, num, msg); }
#define DEFINE_WARNING_A1(num, id, msg, arg1Type, arg1Name)                                  \
  NEVER_INLINE void warning_W##num##_##id(CapricaFileLocation location, arg1Type arg1Name) { \
    warning(location, num, msg, arg1Name);                                                   \
  }
#define DEFINE_WARNING_A2(num, id, msg, arg1Type, arg1Name, arg2Type, arg2Name)                                 \
  NEVER_INLINE void warning_W##num##_##id(CapricaFileLocation location, arg1Type arg1Name, arg2Type arg2Name) { \
    warning(location, num, msg, arg1Name, arg2Name);                                                            \
  }
#define DEFINE_WARNING_A3(num, id, msg, arg1Type, arg1Name, arg2Type, arg2Name, arg3Type, arg3Name) \
  NEVER_INLINE void warning_W##num##_##id(CapricaFileLocation location,                             \
                                          arg1Type arg1Name,                                        \
                                          arg2Type arg2Name,                                        \
                                          arg3Type arg3Name) {                                      \
    warning(location, num, msg, arg1Name, arg2Name, arg3Name);                                      \
  }
#define DEFINE_WARNING_ONCE_A0(num, id, msg)                                           \
  static inline bool m_Warn_##num##_##id_emitted { false };                            \
  NEVER_INLINE void warning_W##num##_##id(CapricaFileLocation location) {              \
    if (!m_Warn_##num##_##id_emitted) {                                                \
      warning(location, num, msg "\n\tFurther {} warnings will be suppressed.", num); \
      m_Warn_##num##_##id_emitted = true;                                              \
    }                                                                                  \
  }
#define DEFINE_WARNING_ONCE_A1(num, id, msg, arg1Type, arg1Name)                                 \
  static inline bool m_Warn_##num##_##id_emitted { false };                                      \
  NEVER_INLINE void warning_W##num##_##id(CapricaFileLocation location, arg1Type arg1Name) {     \
    if (!m_Warn_##num##_##id_emitted) {                                                          \
      warning(location, num, msg "\n\tFurther {} warnings will be suppressed.", arg1Name, num); \
      m_Warn_##num##_##id_emitted = true;                                                        \
    }                                                                                            \
  }
#define DEFINE_WARNING_ONCE_A2(num, id, msg, arg1Type, arg1Name, arg2Type, arg2Name)                            \
  static inline bool m_Warn_##num##_##id_emitted { false };                                                     \
  NEVER_INLINE void warning_W##num##_##id(CapricaFileLocation location, arg1Type arg1Name, arg2Type arg2Name) { \
    if (!m_Warn_##num##_##id_emitted) {                                                                         \
      warning(location, num, msg "\n\tFurther {} warnings will be suppressed.", arg1Name, arg2Name, num);      \
      m_Warn_##num##_##id_emitted = true;                                                                       \
    }                                                                                                           \
  }

  // Warnings 1000-1999 are for warnings that really should be errors but can't be because the base game triggers them.

  DEFINE_WARNING_A1(1000,
                    Strict_Not_All_Control_Paths_Return,
                    "Not all control paths of '{}' return a value.",
                    identifier_ref,
                    functionName)
  DEFINE_WARNING_A0(1001,
                    Strict_Poison_BetaOnly,
                    "The return value of a BetaOnly function cannot be used in a non-BetaOnly context.")
  DEFINE_WARNING_A0(1002,
                    Strict_Poison_DebugOnly,
                    "The return value of a DebugOnly function cannot be used in a non-DebugOnly context.")
  DEFINE_WARNING_A1(
      1003, Strict_None_Implicit_Conversion, "None implicitly converted to '{}'.", std::string_view, destType)
  DEFINE_WARNING_A1(1004,
                    Function_Parameter_Shadows_Property,
                    "The function parameter '{}' shadows script property, using property.",
                    identifier_ref,
                    idName)
  DEFINE_WARNING_A2(1005,
                    Function_Parameter_Shadows_Parent_Property,
                    "The function parameter '{}' shadows parent {} property, using parent property.",
                    identifier_ref,
                    idName,
                    identifier_ref,
                    parentName)

  // Not a base game warning, but some ubiquitous libraries fall prey to this.
  DEFINE_WARNING_A1(1006, 
                    Strict_Keyword_Identifiers, 
                    "'{}' is a keyword (e.g. Int, Function) but is used as an identifier.",
                    identifier_ref, 
                    idName)

  // Warnings 2000-2199 are for engine imposed limitations.

  DEFINE_WARNING_A2(2001,
                    EngineLimits_ArrayLength,
                    "Attempting to create an array with {} elements, but the engine limit is {} elements.",
                    size_t,
                    count,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A2(2002,
                    EngineLimits_PexFile_UserFlagCount,
                    "There are {} distinct user flags defined, but the engine limit is {} flags.",
                    size_t,
                    count,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A3(2003,
                    EngineLimits_PexFunction_ParameterCount,
                    "There are {} parameters declared for the '{}' function, but the engine limit is {} parameters.",
                    size_t,
                    count,
                    identifier_ref,
                    functionName,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A2(2004,
                    EngineLimits_PexObject_EmptyStateFunctionCount,
                    "There are {} functions in the empty state, but the engine limit is {} functions.",
                    size_t,
                    count,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A2(2005,
                    EngineLimits_PexObject_InitialValueCount,
                    "There are {} variables with initial values, but the engine limit is {} initial values.",
                    size_t,
                    count,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A2(2006,
                    EngineLimits_PexObject_NamedStateCount,
                    "There are {} named states in this object, but the engine limit is {} named states.",
                    size_t,
                    count,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A2(2007,
                    EngineLimits_PexObject_PropertyCount,
                    "There are {} properties in this object, but the engine limit is {} properties.",
                    size_t,
                    count,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A2(2008,
                    EngineLimits_PexObject_StaticFunctionCount,
                    "There are {} static functions in this object, but the engine limit is {} static functions.",
                    size_t,
                    count,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A2(2009,
                    EngineLimits_PexObject_VariableCount,
                    "There are {} variables in this object, but the engine limit is {} variables.",
                    size_t,
                    count,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A3(2010,
                    EngineLimits_PexState_FunctionCount,
                    "There are {} functions in the '{}' state, but the engine limit is {} functions in a named state.",
                    size_t,
                    count,
                    identifier_ref,
                    stateName,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A2(2011,
                    EngineLimits_PexObject_GuardCount,
                    "There are {} guards in this object, but the engine limit is {} guards.",
                    size_t,
                    count,
                    size_t,
                    engineMax)

  // Warnings 4000-6000 are for general Papyrus Script warnings.

  DEFINE_WARNING_A2(4001,
                    Unecessary_Cast,
                    "Unnecessary cast from '{}' to '{}'.",
                    identifier_ref,
                    sourceType,
                    identifier_ref,
                    targetType)
  DEFINE_WARNING_A1(
      4003, State_Doesnt_Exist, "The state '{}' doesn't exist in this context.", identifier_ref, stateName)
  DEFINE_WARNING_A1(4004,
                    Unreferenced_Script_Variable,
                    "The script variable '{}' is declared but never used.",
                    identifier_ref,
                    variableName)
  DEFINE_WARNING_A1(4005,
                    Unwritten_Script_Variable,
                    "The script variable '{}' is not initialized, and is never written to.",
                    identifier_ref,
                    variableName)
  DEFINE_WARNING_A1(4006,
                    Script_Variable_Only_Written,
                    "The script variable '{}' is only ever written to.",
                    identifier_ref,
                    variableName)
  DEFINE_WARNING_A1(4007,
                    Script_Variable_Initialized_Never_Used,
                    "The script variable '{}' is initialized but is never used.",
                    identifier_ref,
                    variableName)

  // TODO: Starfield: reevaluate when CK comes out
  // Warnings 6001-7000 are for warning about use of experimental syntax subject to change.

  DEFINE_WARNING_ONCE_A0(
      6001,
      Experimental_Syntax_ArrayGetAllMatchingStructs,
      "The syntax of the 'ArrayGetAllMatchingStructs' function is experimental and subject to change.")
  DEFINE_WARNING_ONCE_A0(6002,
                         Experimental_Syntax_Lock,
                         "The syntax for Guard/EndGuard is experimental and subject to change.")
  DEFINE_WARNING_ONCE_A0(6003,
                         Experimental_Syntax_TryLock,
                         "The syntax for TryGuard/EndGuard is experimental and subject to change.")
  DEFINE_WARNING_ONCE_A2(6004,
                         Experimental_Downcast_Arrays,
                         "Downcasting Arrays ('{}') to ('{}') is experimental and subject to change.",
                         identifier_ref,
                         sourceType,
                         identifier_ref,
                         targetType);

  // Warnings 7000-8000 are Skyrim warnings and for emitting warnings when attempting to emulate the truly *cursed*
  // behavior of Skyrim's PCompiler

  DEFINE_WARNING_A3(7000,
                    Skyrim_Unknown_Event_On_Non_Native_Class,
                    "Unknown Event ('{}') on non-native class '{}' that extends from '{}'.",
                    identifier_ref,
                    eventName,
                    identifier_ref,
                    sourceType,
                    identifier_ref,
                    parentType);
  DEFINE_WARNING_A3(7001,
                    Skyrim_Child_Variable_Shadows_Parent_Property,
                    "Object variable '{}' on class '{}' shadows parent class '{}' property, using parent property",
                    identifier_ref,
                    varName,
                    identifier_ref,
                    sourceType,
                    identifier_ref,
                    parentType);
  DEFINE_WARNING_A2(7002,
                    Skyrim_Local_Variable_Shadows_Parent_Property,
                    "Local variable '{}' shadows parent class '{}' property, using parent property",
                    identifier_ref,
                    varName,
                    identifier_ref,
                    parentType)
  DEFINE_WARNING_A1(7003,
                    Skyrim_Local_Use_Before_Declaration,
                    "Local variable '{}' is used before it is declared.",
                    identifier_ref,
                    varName);
  // TODO: Check for this in semantic parsing, we only catch it when emitting pex
  DEFINE_WARNING_A1(7004,
                    Skyrim_Assignment_Of_Void_Call_Result,
                    "Assignment of result from void call '{}'.",
                    identifier_ref,
                    functionName);
  DEFINE_WARNING_A1(
      7005, Skyrim_Casting_None_Call_Result, "Casting None method call result to type '{}'", std::string_view, type);

#undef DEFINE_WARNING_A1
#undef DEFINE_WARNING_A2
#undef DEFINE_WARNING_A3

private:
  allocators::FileOffsetPool lineOffsets {};

  NEVER_INLINE
  static void pushToErrorStream(std::string&& msg, bool isError = false);
  NEVER_INLINE
  bool isWarningError(CapricaFileLocation location, size_t warningNumber) const;
  NEVER_INLINE
  bool isWarningEnabled(CapricaFileLocation location, size_t warningNumber) const;
  NEVER_INLINE
  std::string formatLocation(CapricaFileLocation loc);
  NEVER_INLINE
  static void maybePushMessage(CapricaReportingContext* ctx,
                               CapricaFileLocation* loc,
                               std::string_view msgType,
                               size_t warnNum,
                               const std::string& msg,
                               bool forceAsError = false);

  template <typename... Args>
  ALWAYS_INLINE void
  warning(CapricaFileLocation location, size_t warningNumber, fmt::format_string<Args...> msg, Args&&... args) {
    // TODO: fix Imports hack
    if (!m_QuietWarnings)
      maybePushMessage(this, &location, "", warningNumber, fmt::format(msg, std::forward<Args>(args)...));
  }
};

}
