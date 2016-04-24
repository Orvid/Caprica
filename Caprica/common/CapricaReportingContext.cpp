#include <common/CapricaReportingContext.h>

#include <cassert>
#include <iostream>

#include <common/CapricaConfig.h>

#include <Windows.h>

namespace caprica {

void CapricaReportingContext::pushToErrorStream(std::string&& msg) {
  if (!conf::Performance::performanceTestMode) {
    std::cout.flush();
    std::cerr << msg << std::endl;
  }
}

void CapricaReportingContext::breakIfDebugging() {
  if (IsDebuggerPresent()) {
    __debugbreak();
  }
}

void CapricaReportingContext::exitIfErrors() {
  if (errorCount > 0) {
    pushToErrorStream("Compilation of '" + filename + "' failed; " + std::to_string(warningCount) + " warnings and " + std::to_string(errorCount) + " errors were encountered.");
    throw std::runtime_error("");
  }
}

bool CapricaReportingContext::isWarningError(CapricaFileLocation location, size_t warningNumber) const {
  if (warningNumber >= 2000 && warningNumber <= 2200) {
    return !conf::EngineLimits::ignoreLimits && conf::Warnings::warningsToIgnore.count(warningNumber) == 0;
  }
  return conf::Warnings::treatWarningsAsErrors || conf::Warnings::warningsToHandleAsErrors.count(warningNumber);
}

bool CapricaReportingContext::isWarningEnabled(CapricaFileLocation location, size_t warningNumber) const {
  if (conf::Warnings::warningsToHandleAsErrors.count(warningNumber))
    return true;
  if (conf::Warnings::warningsToEnable.count(warningNumber))
    return true;
  if (conf::Warnings::warningsToIgnore.count(warningNumber))
    return false;
  if (warningNumber >= 2000 && warningNumber <= 2200)
    return !conf::EngineLimits::ignoreLimits;
  return !conf::Warnings::disableAllWarnings;
}

size_t CapricaReportingContext::getLocationLine(CapricaFileLocation location) const {
  // TODO: B-Tree search.
  for (size_t i = lineOffsets.size() - 1; i >= 0; i--) {
    if (location.fileOffset >= lineOffsets[i]) {
      return i + 1;
    }
  }
  CapricaReportingContext::logicalFatal("Unable to locate line at offset %zu.", location.fileOffset);
}

std::string CapricaReportingContext::formatLocation(CapricaFileLocation loc) const {
  std::string ret = filename + " (";
  auto line = getLocationLine(loc);
  ret += std::to_string(line) + ", ";
  ret += std::to_string(loc.fileOffset - lineOffsets[line - 1] + 1) + ")";
  return ret;
}

}
