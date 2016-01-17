#pragma once

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <pex/PexUserFlags.h>

namespace caprica { namespace pex {

struct PexAsmWriter final
{
  // The indent level. Yes, this spelling is deliberate.
  size_t ident{ 0 };

  explicit PexAsmWriter(std::ostream& dest) : strm(dest) { }
  PexAsmWriter(const PexAsmWriter&) = delete;
  ~PexAsmWriter() = default;

  template<typename T>
  void writeKV(const char* key, T val) {
    static_assert(false, "Unknown type for the value!");
  }

  template<>
  void writeKV(const char* key, time_t val) {
    ensureIndent();
    // TODO: Add a comment output of the times in the local time.
    strm << '.' << key << ' ' << (unsigned long long)val;
    writeln();
  }

  template<>
  void writeKV(const char* key, std::string val) {
    ensureIndent();
    strm << '.' << key << " \"" << escapeString(val) << "\"";
    writeln();
  }

  template<>
  void writeKV(const char* key, PexUserFlags val) {
    ensureIndent();
    strm << '.' << key << " " << val.data;
    writeln();
  }

  template<typename... Args>
  void write(const std::string& msg, Args&&... args) {
    ensureIndent();
    strm << formatString(msg, args...);
  }

  template<typename... Args>
  void writeln(const std::string& msg, Args&&... args) {
    ensureIndent();
    strm << formatString(msg, args...);
    writeln();
  }

  void writeln() {
    haveIndented = false;
    strm << std::endl;
  }

  static std::string escapeString(const std::string& str) {
    std::ostringstream dest;
    
    for (auto c : str) {
      switch (c) {
        case '\n':
          dest.put('\\');
          dest.put('n');
          break;
        case '\t':
          dest.put('\\');
          dest.put('t');
          break;
        case '"':
          dest.put('\\');
          dest.put('"');
          break;
        case '\\':
          dest.put('\\');
          dest.put('\\');
          break;
        default:
          dest.put(c);
          break;
      }
    }

    return dest.str();
  }

private:
  bool haveIndented{ false };
  std::ostream& strm;

  void ensureIndent() {
    if (!haveIndented) {
      for (size_t i = 0; i < ident; i++) {
        strm << "  ";
      }
      haveIndented = true;
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

}}
