#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>

namespace caprica {

struct CapricaFileLocation
{
  std::string filename{ "" };
  size_t line{ 0 };
  size_t column{ 0 };

  CapricaFileLocation(std::string fn, size_t ln, size_t col) : filename(fn), line(ln), column(col) { }
  CapricaFileLocation(const CapricaFileLocation& other) = default;
  ~CapricaFileLocation() = default;

  void nextLine() {
    line++;
    column = 0;
  }

  std::string buildString() const {
    std::ostringstream str;
    str << filename << "(" << line << "," << column << ")";
    return str.str();
  }
};

}
