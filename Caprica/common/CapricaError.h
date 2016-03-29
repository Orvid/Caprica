#pragma once

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include <common/CapricaConfig.h>
#include <common/CapricaFileLocation.h>
#include <common/UtilMacros.h>

namespace caprica {

struct CapricaError abstract
{
  CapricaError() = delete;
  CapricaError(const CapricaError&) = delete;
  ~CapricaError() = delete;

  static size_t warningCount;
  static size_t errorCount;

  struct Warning abstract
  {
    Warning() = delete;
    Warning(const Warning&) = delete;
    ~Warning() = delete;

#define DEFINE_WARNING_A1(num, id, msg, arg1Type, arg1Name) \
NEVER_INLINE static void W##num##_##id##(const CapricaFileLocation& location, arg1Type arg1Name) { CapricaError::warning(num, location, msg, arg1Name); }
#define DEFINE_WARNING_A2(num, id, msg, arg1Type, arg1Name, arg2Type, arg2Name) \
NEVER_INLINE static void W##num##_##id##(const CapricaFileLocation& location, arg1Type arg1Name, arg2Type arg2Name) { CapricaError::warning(num, location, msg, arg1Name, arg2Name); }

    DEFINE_WARNING_A2(4001, Unecessary_Cast, "Unecessary cast from '%s' to '%s'.", const char*, sourceType, const char*, targetType)
    DEFINE_WARNING_A1(4002, Duplicate_Import, "Duplicate import of '%s'.", const char*, importName)
    DEFINE_WARNING_A1(4003, State_Doesnt_Exist, "The state '%s' doesn't exist in this context.", const char*, stateName)
    DEFINE_WARNING_A1(4004, Unreferenced_Script_Variable, "The script variable '%s' is declared but never used.", const char*, variableName)
    DEFINE_WARNING_A1(4005, Unwritten_Script_Variable, "The script variable '%s' is not initialized, and is never written to.", const char*, variableName)
    DEFINE_WARNING_A1(4006, Script_Variable_Only_Written, "The script variable '%s' is only ever written to.", const char*, variableName)
    DEFINE_WARNING_A1(4007, Script_Variable_Initialized_Never_Used, "The script variable '%s' is initialized but is never used.", const char*, variableName)

#undef DEFINE_WARNING_A1
#undef DEFINE_WARNING_A2
  };

  NEVER_INLINE
  static void exitIfErrors();
  NEVER_INLINE
  static bool isWarningEnabled(size_t warningNumber);

  template<typename... Args>
  NEVER_INLINE
  static void error(const CapricaFileLocation& location, const std::string& msg, Args&&... args) {
    errorCount++;
    std::cerr << formatString(location, "Error", msg, args...) << std::endl;
  }

  template<typename... Args>
  [[noreturn]] NEVER_INLINE
  static void fatal(const CapricaFileLocation& location, const std::string& msg, Args&&... args) {
    auto str = formatString(location, "Fatal Error", msg, args...);
    std::cerr << str << std::endl;
    throw std::runtime_error("");
  }

  // The difference between this and fatal is that this is intended for places
  // where the logic of Caprica itself has failed, and a location in a source
  // file is likely not available.
  template<typename... Args>
  [[noreturn]] NEVER_INLINE
  static void logicalFatal(const std::string& msg, Args&&... args) {
    auto str = formatString("Fatal Error", msg, args...);
    std::cerr << str << std::endl;
    throw std::runtime_error("");
  }

private:
  template<typename... Args>
  NEVER_INLINE
  static void warning(size_t warningNumber, const CapricaFileLocation& location, const std::string& msg, Args&&... args) {
    if (isWarningEnabled(warningNumber)) {
      warningCount++;
      if (CapricaConfig::treatWarningsAsErrors || CapricaConfig::warningsToHandleAsErrors.count(warningNumber)) {
        errorCount++;
        std::cerr << formatString(location, "Error (Warning as Error) W" + std::to_string(warningNumber), msg, args...) << std::endl;
      } else {
        std::cerr << formatString(location, "Warning W" + std::to_string(warningNumber), msg, args...) << std::endl;
      }
    }
  }

  template<typename... Args>
  NEVER_INLINE
  static std::string formatString(const CapricaFileLocation& location, const std::string& msgType, const std::string& msg, Args&&... args) {
    if (sizeof...(args)) {
      size_t size = std::snprintf(nullptr, 0, msg.c_str(), args...) + 1;
      std::unique_ptr<char[]> buf(new char[size]);
      std::snprintf(buf.get(), size, msg.c_str(), args...);
      return location.buildString() + ": " + msgType + ": " + std::string(buf.get(), buf.get() + size - 1);
    } else {
      return location.buildString() + ": " + msgType + ": " + msg;
    }
  }

  template<typename... Args>
  NEVER_INLINE
  static std::string formatString(const std::string& msgType, const std::string& msg, Args&&... args) {
    if (sizeof...(args)) {
      size_t size = std::snprintf(nullptr, 0, msg.c_str(), args...) + 1;
      std::unique_ptr<char[]> buf(new char[size]);
      std::snprintf(buf.get(), size, msg.c_str(), args...);
      return msgType + ": " + std::string(buf.get(), buf.get() + size - 1);
    } else {
      return msgType + ": " + msg;
    }
  }
};

}
