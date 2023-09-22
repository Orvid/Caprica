#pragma once

#include <cstdint>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <common/allocators/FileOffsetPool.h>
#include <common/CapricaFileLocation.h>
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
  void pushNextLineOffset(CapricaFileLocation location) { lineOffsets.push_back(location.fileOffset); }

  NEVER_INLINE
  static void breakIfDebugging();
  NEVER_INLINE
  void exitIfErrors();

  template <typename... Args>
  NEVER_INLINE void error(CapricaFileLocation location, const char* msg, Args&&... args) {
    errorCount++;
    maybePushMessage(this, &location, "Error", 0, formatString(msg, std::forward<Args>(args)...), true);
    breakIfDebugging();
  }

  template <typename... Args>
  [[noreturn]] NEVER_INLINE void fatal(CapricaFileLocation location, const char* msg, Args&&... args) {
    maybePushMessage(this, &location, "Fatal Error", 0, formatString(msg, std::forward<Args>(args)...), true);
    throw std::runtime_error("");
  }

  // The difference between this and fatal is that this is intended for places
  // where the logic of Caprica itself has failed, and a location in a source
  // file is likely not available.
  template <typename... Args>
  [[noreturn]] NEVER_INLINE static void logicalFatal(const char* msg, Args&&... args) {
    maybePushMessage(nullptr, nullptr, "Fatal Error", 0, formatString(msg, std::forward<Args>(args)...), true);
    throw std::runtime_error("");
  }

  template <typename... Args>
  NEVER_INLINE static std::string formatString(const char* msg, Args&&... args) {
    constexpr bool haveArgs = sizeof...(args) != 0;
    if (haveArgs) {
      size_t size = std::snprintf(nullptr, 0, msg, std::forward<Args>(args)...) + 1;
      std::unique_ptr<char[]> buf(new char[size]);
      std::snprintf(buf.get(), size, msg, std::forward<Args>(args)...);
      return std::string(buf.get(), buf.get() + size - 1);
    } else {
      return msg;
    }
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
#define DEFINE_WARNING_ONCE_A0(num, id, msg)                                             \
  static inline bool m_Warn_##num##_##id_emitted { false };                              \
  NEVER_INLINE void warning_W##num##_##id(CapricaFileLocation location) {                \
    if (!m_Warn_##num##_##id_emitted) {                                                  \
      warning(location, num, "%s\n\tFurther %d warnings will be suppressed.", msg, num); \
      m_Warn_##num##_##id_emitted = true;                                                \
    }                                                                                    \
  }
#define DEFINE_WARNING_ONCE_A1(num, id, msg, arg1Type, arg1Name)                             \
  static inline bool m_Warn_##num##_##id_emitted { false };                                  \
  NEVER_INLINE void warning_W##num##_##id(CapricaFileLocation location, arg1Type arg1Name) { \
    if (!m_Warn_##num##_##id_emitted) {                                                      \
      warning(location,                                                                      \
              num,                                                                           \
              "%s\n\tFurther %d warnings will be suppressed.",                               \
              formatString(msg, arg1Name).c_str(),                                           \
              num);                                                                          \
      m_Warn_##num##_##id_emitted = true;                                                    \
    }                                                                                        \
  }
#define DEFINE_WARNING_ONCE_A2(num, id, msg, arg1Type, arg1Name, arg2Type, arg2Name)                            \
  static inline bool m_Warn_##num##_##id_emitted { false };                                                     \
  NEVER_INLINE void warning_W##num##_##id(CapricaFileLocation location, arg1Type arg1Name, arg2Type arg2Name) { \
    if (!m_Warn_##num##_##id_emitted) {                                                                         \
      warning(location,                                                                                         \
              num,                                                                                              \
              "%s\n\tFurther %d warnings will be suppressed.",                                                  \
              formatString(msg, arg1Name, arg2Name).c_str(),                                                    \
              num);                                                                                             \
      m_Warn_##num##_##id_emitted = true;                                                                       \
    }                                                                                                           \
  }

  // Warnings 1000-1999 are for warnings that really should be errors but can't be because the base game triggers them.
  DEFINE_WARNING_A1(1000,
                    Strict_Not_All_Control_Paths_Return,
                    "Not all control paths of '%s' return a value.",
                    const char*,
                    functionName)
  DEFINE_WARNING_A0(1001,
                    Strict_Poison_BetaOnly,
                    "The return value of a BetaOnly function cannot be used in a non-BetaOnly context.")
  DEFINE_WARNING_A0(1002,
                    Strict_Poison_DebugOnly,
                    "The return value of a DebugOnly function cannot be used in a non-DebugOnly context.")
  DEFINE_WARNING_A1(
      1003, Strict_None_Implicit_Conversion, "None implicitly converted to '%s'.", const char*, destTypeName)
  DEFINE_WARNING_A1(1004,
                    Function_Parameter_Shadows_Property,
                    "The function parameter '%s' shadows script property, using property.",
                    const char*,
                    idName)
  DEFINE_WARNING_A2(1005,
                    Function_Parameter_Shadows_Parent_Property,
                    "The function parameter '%s' shadows parent %s property, using parent property.",
                    const char*,
                    idName,
                    const char*,
                    parentName)

  // Warnings 2000-2199 are for engine imposed limitations.
  DEFINE_WARNING_A2(2001,
                    EngineLimits_ArrayLength,
                    "Attempting to create an array with %zu elements, but the engine limit is %zu elements.",
                    size_t,
                    count,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A2(2002,
                    EngineLimits_PexFile_UserFlagCount,
                    "There are %zu distinct user flags defined, but the engine limit is %zu flags.",
                    size_t,
                    count,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A3(2003,
                    EngineLimits_PexFunction_ParameterCount,
                    "There are %zu parameters declared for the '%s' function, but the engine limit is %zu parameters.",
                    size_t,
                    count,
                    const char*,
                    functionName,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A2(2004,
                    EngineLimits_PexObject_EmptyStateFunctionCount,
                    "There are %zu functions in the empty state, but the engine limit is %zu functions.",
                    size_t,
                    count,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A2(2005,
                    EngineLimits_PexObject_InitialValueCount,
                    "There are %zu variables with initial values, but the engine limit is %zu intial values.",
                    size_t,
                    count,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A2(2006,
                    EngineLimits_PexObject_NamedStateCount,
                    "There are %zu named states in this object, but the engine limit is %zu named states.",
                    size_t,
                    count,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A2(2007,
                    EngineLimits_PexObject_PropertyCount,
                    "There are %zu properties in this object, but the engine limit is %zu properties.",
                    size_t,
                    count,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A2(2008,
                    EngineLimits_PexObject_StaticFunctionCount,
                    "There are %zu static functions in this object, but the engine limit is %zu static functions.",
                    size_t,
                    count,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A2(2009,
                    EngineLimits_PexObject_VariableCount,
                    "There are %zu variables in this object, but the engine limit is %zu variables.",
                    size_t,
                    count,
                    size_t,
                    engineMax)
  DEFINE_WARNING_A3(
      2010,
      EngineLimits_PexState_FunctionCount,
      "There are %zu functions in the '%s' state, but the engine limit is %zu functions in a named state.",
      size_t,
      count,
      const char*,
      stateName,
      size_t,
      engineMax)
  DEFINE_WARNING_A2(2011,
                    EngineLimits_PexObject_GuardCount,
                    "There are %zu guards in this object, but the engine limit is %zu guards.",
                    size_t,
                    count,
                    size_t,
                    engineMax)

  // Warnings 4000-6000 are for general Papyrus Script warnings.
  DEFINE_WARNING_A2(
      4001, Unecessary_Cast, "Unecessary cast from '%s' to '%s'.", const char*, sourceType, const char*, targetType)
  DEFINE_WARNING_A1(4003, State_Doesnt_Exist, "The state '%s' doesn't exist in this context.", const char*, stateName)
  DEFINE_WARNING_A1(4004,
                    Unreferenced_Script_Variable,
                    "The script variable '%s' is declared but never used.",
                    const char*,
                    variableName)
  DEFINE_WARNING_A1(4005,
                    Unwritten_Script_Variable,
                    "The script variable '%s' is not initialized, and is never written to.",
                    const char*,
                    variableName)
  DEFINE_WARNING_A1(4006,
                    Script_Variable_Only_Written,
                    "The script variable '%s' is only ever written to.",
                    const char*,
                    variableName)
  DEFINE_WARNING_A1(4007,
                    Script_Variable_Initialized_Never_Used,
                    "The script variable '%s' is initialized but is never used.",
                    const char*,
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
                         "Downcasting Arrays ('%s') to ('%s') is experimental and subject to change.",
                         const char*,
                         sourceType,
                         const char*,
                         targetType);

  // Warnings 7000-8000 are Skyrim warnings and for emitting warnings when attempting to emulate the truly *cursed*
  // behavior of Skyrim's PCompiler
  DEFINE_WARNING_A3(7000,
                    Skyrim_Unknown_Event_On_Non_Native_Class,
                    "Unknown Event ('%s') on non-native class '%s' that extends from '%s'.",
                    const char*,
                    eventName,
                    const char*,
                    sourceType,
                    const char*,
                    parentType);
  DEFINE_WARNING_A3(7001,
                    Skyrim_Child_Variable_Shadows_Parent_Property,
                    "Object variable '%s' on class '%s' shadows parent class '%s' property, using parent property",
                    const char*,
                    varName,
                    const char*,
                    sourceType,
                    const char*,
                    parentType);
  DEFINE_WARNING_A2(7002,
                    Skyrim_Local_Variable_Shadows_Parent_Property,
                    "Local variable '%s' shadows parent class '%s' property, using parent property",
                    const char*,
                    varName,
                    const char*,
                    parentType)
  DEFINE_WARNING_A1(7003,
                    Skyrim_Local_Use_Before_Declaration,
                    "Local variable '%s' is used before it is declared.",
                    const char*,
                    varName);
  // TODO: Check for this in semantic parsing, we only catch it when emitting pex
  DEFINE_WARNING_A1(7004,
                    Skyrim_Assignment_Of_Void_Call_Result,
                    "Assignment of result from void call '%s'.",
                    const char*,
                    functionName);
  DEFINE_WARNING_A1(
      7005, Skyrim_Casting_None_Call_Result, "Casting None method call result to type '%s'", const char*, type);

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
                               const char* msgType,
                               size_t warnNum,
                               const std::string& msg,
                               bool forceAsError = false);

  template <typename... Args>
  ALWAYS_INLINE void warning(CapricaFileLocation location, size_t warningNumber, const char* msg, Args&&... args) {
    // TODO: fix Imports hack
    if (!m_QuietWarnings)
      maybePushMessage(this, &location, nullptr, warningNumber, formatString(msg, std::forward<Args>(args)...));
  }
};

}
