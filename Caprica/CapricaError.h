#pragma once

#include <memory>
#include <string>

#include <papyrus/parser/PapyrusFileLocation.h>

namespace caprica {

struct CapricaError abstract
{

  template<typename... Args>
  [[noreturn]]
  static void fatal(const papyrus::parser::PapyrusFileLocation& location, const std::string& msg, Args&&... args) {
    throw std::runtime_error(formatString(location, msg, args...));
  }

  // The difference between this and fatal is that this is intended for places
  // where the logic of Caprica itself has failed, and a location in a source
  // file is likely not available.
  template<typename... Args>
  [[noreturn]]
  static void logicalFatal(const std::string& msg, Args&&... args) {
    throw std::runtime_error(formatString(msg, args...));
  }

private:
  template<typename... Args>
  static std::string formatString(const papyrus::parser::PapyrusFileLocation& location, const std::string& msg, Args&&... args) {
    if (sizeof...(args)) {
      size_t size = std::snprintf(nullptr, 0, msg.c_str(), args...) + 1;
      std::unique_ptr<char[]> buf(new char[size]);
      std::snprintf(buf.get(), size, msg.c_str(), args...);
      return location.buildString() + ": " + std::string(buf.get(), buf.get() + size - 1);
    } else {
      return location.buildString() + ": " + msg;
    }
  }

  template<typename... Args>
  static std::string formatString(const std::string& msg, Args&&... args) {
    if (sizeof...(args)) {
      size_t size = std::snprintf(nullptr, 0, msg.c_str(), args...) + 1;
      std::unique_ptr<char[]> buf(new char[size]);
      std::snprintf(buf.get(), size, msg.c_str(), args...);
      return std::string(buf.get(), buf.get() + size - 1);
    } else {
      return msg;
    }
  }
};

}
