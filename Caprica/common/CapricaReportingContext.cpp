#include <common/CapricaReportingContext.h>

#include <cassert>
#include <iostream>

#include <common/CapricaConfig.h>

#include <Windows.h>

namespace caprica {

void CapricaReportingContext::pushToErrorStream(std::string&& msg, bool isError) {
  if (!conf::Performance::performanceTestMode || isError) {
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

size_t CapricaReportingContext::getLocationLine(CapricaFileLocation location, size_t lastLineHint) {
  if (!lineOffsets.size())
    CapricaReportingContext::logicalFatal("Unable to locate line at offset %zu.", location.fileOffset);
  if (lastLineHint != 0) {
    if (location.fileOffset >= lineOffsets.at(lastLineHint - 1)) {
      return lastLineHint + 1;
    }
    if (lastLineHint + 1 < lineOffsets.size()) {
      if (location.fileOffset >= lineOffsets.at(lastLineHint - 1)) {
        return lastLineHint + 1;
      }
    }
  }
  auto a = std::lower_bound(lineOffsets.begin(), lineOffsets.end(), location.fileOffset);
  if (a == lineOffsets.end())
    CapricaReportingContext::logicalFatal("Unable to locate line at offset %zu.", location.fileOffset);
  return std::distance(lineOffsets.begin(), a);
}

std::string CapricaReportingContext::formatLocation(CapricaFileLocation loc) {
  std::string ret = filename + " (";
  auto line = getLocationLine(loc);
  ret += std::to_string(line) + ", ";
  ret += std::to_string(loc.fileOffset - lineOffsets.at(line - 1) + 1) + ")";
  return ret;
}

void CapricaReportingContext::maybePushMessage(CapricaReportingContext* ctx, CapricaFileLocation* location, const char* msgType, size_t warningNumber, const std::string& msg, bool forceAsError) {
  if (warningNumber != 0) {
    if (ctx->isWarningEnabled(*location, warningNumber)) {
      if (ctx->isWarningError(*location, warningNumber)) {
        ctx->errorCount++;
        pushToErrorStream(ctx->formatLocation(*location) + ": Error W" + std::to_string(warningNumber) + ": " + msg, true);
      } else {
        ctx->warningCount++;
        pushToErrorStream(ctx->formatLocation(*location) + ": Warning W" + std::to_string(warningNumber) + ": " + msg);
      }
    }
  } else if (location != nullptr) {
    pushToErrorStream(ctx->formatLocation(*location) + ": " + msgType + ": " + msg, forceAsError);
  } else {
    pushToErrorStream(std::string(msgType) + ": " + msg, forceAsError);
  }
}

}
