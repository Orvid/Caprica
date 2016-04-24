#include <common/CapricaReportingContext.h>

#include <cassert>
#include <iostream>

#include <common/CapricaConfig.h>

#include <Windows.h>

namespace caprica {

void CapricaReportingContext::pushToErrorStream(std::string&& msg) {
  if (!CapricaConfig::performanceTestMode) {
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
    return !CapricaConfig::EngineLimits::ignoreLimits && CapricaConfig::warningsToIgnore.count(warningNumber) == 0;
  }
  return CapricaConfig::treatWarningsAsErrors || CapricaConfig::warningsToHandleAsErrors.count(warningNumber);
}

bool CapricaReportingContext::isWarningEnabled(CapricaFileLocation location, size_t warningNumber) const {
  return CapricaConfig::warningsToIgnore.count(warningNumber) == 0 && CapricaConfig::warningsToHandleAsErrors.count(warningNumber) == 0;
}

size_t CapricaReportingContext::getLocationLine(CapricaFileLocation location) const {
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
