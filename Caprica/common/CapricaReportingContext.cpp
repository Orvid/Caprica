#include <common/CapricaReportingContext.h>

#include <algorithm>
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
  if (IsDebuggerPresent())
    __debugbreak();
}

void CapricaReportingContext::exitIfErrors() {
  if (errorCount > 0) {
    pushToErrorStream(fmt::format("Compilation of '{}' failed; {} warnings and {} errors were encountered.", filename, warningCount, errorCount));
    throw std::runtime_error("");
  }
}

bool CapricaReportingContext::isWarningError(CapricaFileLocation /* location */, size_t warningNumber) const {
  // TODO: Support disabling warnings for specific sections of code.
  if (warningNumber >= 2000 && warningNumber <= 2200)
    return !conf::EngineLimits::ignoreLimits && conf::Warnings::warningsToIgnore.count(warningNumber) == 0;
  return conf::Warnings::treatWarningsAsErrors || conf::Warnings::warningsToHandleAsErrors.count(warningNumber);
}

bool CapricaReportingContext::isWarningEnabled(CapricaFileLocation /* location */, size_t warningNumber) const {
  // TODO: Support disabling warnings for specific sections of code.
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
    CapricaReportingContext::logicalFatal("Unable to locate line at offset {}.", location.startOffset);
  auto a = std::lower_bound(lineOffsets.begin(), lineOffsets.end(), location.startOffset);
  if (a == lineOffsets.end()) {
    if (lastLineHint != 0) {
      if (location.startOffset >= lineOffsets.at(lastLineHint - 1))
        return lastLineHint + 1;
      if (lastLineHint + 1 < lineOffsets.size()) {
        if (location.startOffset >= lineOffsets.at(lastLineHint - 1))
          return lastLineHint + 1;
      }
    }
    // TODO: Fix line offsets during parsing for reals, remove this hack
    // maybePushMessage(this, nullptr, "Warning:", 0, fmt::format("Unable to locate line at offset {}, using last known line {}...", location.startOffset, lineOffsets.size()), true);
    return lineOffsets.size();
    // CapricaReportingContext::logicalFatal("Unable to locate line at offset {}.", location.startOffset);
  }
  return std::distance(lineOffsets.begin(), a);
}

std::string CapricaReportingContext::formatLocation(CapricaFileLocation loc) {
  auto line = getLocationLine(loc);
  auto column = loc.startOffset - lineOffsets.at(line - 1) + 1;
  auto columnEnd = loc.endOffset - loc.startOffset + column;
  return fmt::format("{} ({}, {}:{})", filename, line, column, columnEnd);
}

void CapricaReportingContext::maybePushMessage(CapricaReportingContext* ctx,
                                               CapricaFileLocation* location,
                                               std::string_view msgType,
                                               size_t warningNumber,
                                               const std::string& msg,
                                               bool forceAsError) {
  if (warningNumber != 0) {
    if (ctx->isWarningEnabled(*location, warningNumber)) {
      if (ctx->isWarningError(*location, warningNumber)) {
        ctx->errorCount++;
        pushToErrorStream(fmt::format("{}: Error W{}: {}", ctx->formatLocation(*location), warningNumber, msg), true);
      } else {
        ctx->warningCount++;
        pushToErrorStream(fmt::format("{}: Warning W{}: {}", ctx->formatLocation(*location), warningNumber, msg));
      }
    }
  } else if (location != nullptr) {
    pushToErrorStream(fmt::format("{}: {}: {}", ctx->formatLocation(*location), msgType, msg), forceAsError);
  } else {
    pushToErrorStream(fmt::format("{}: {}", msgType, msg), forceAsError);
  }
}

}
