#pragma once

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include <common/CapricaFileLocation.h>

namespace caprica {

struct CapricaError abstract
{
  template<typename... Args>
  static void error(const CapricaFileLocation& location, const std::string& msg, Args&&... args) {
    // TODO: Make sure the next stage doesn't occur if this is called.
    std::cerr << formatString(location, "Error", msg, args...) << std::endl;
  }

  template<typename... Args>
  [[noreturn]]
  static void fatal(const CapricaFileLocation& location, const std::string& msg, Args&&... args) {
    auto str = formatString(location, "Fatal Error", msg, args...);
    std::cerr << str << std::endl;
    throw std::runtime_error(str);
  }

  // The difference between this and fatal is that this is intended for places
  // where the logic of Caprica itself has failed, and a location in a source
  // file is likely not available.
  template<typename... Args>
  [[noreturn]]
  static void logicalFatal(const std::string& msg, Args&&... args) {
    auto str = formatString("Fatal Error", msg, args...);
    std::cerr << str << std::endl;
    throw std::runtime_error(str);
  }

private:
  template<typename... Args>
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
