#include <common/CapricaError.h>

#include <common/CapricaConfig.h>

#include <Windows.h>

namespace caprica {

size_t CapricaError::warningCount{ 0 };
size_t CapricaError::errorCount{ 0 };

void CapricaError::breakIfDebugging() {
  if (IsDebuggerPresent()) {
    __debugbreak();
  }
}

void CapricaError::exitIfErrors() {
  if (errorCount > 0) {
    std::cout.flush();
    std::cerr << "Compilation failed, " << warningCount << " warnings and " << errorCount << " errors were encountered." << std::endl;
    std::cerr.flush();
    throw std::runtime_error("");
  }
}

bool CapricaError::isWarningError(size_t warningNumber) {
  if (warningNumber >= 2000 && warningNumber <= 2200) {
    return !CapricaConfig::EngineLimits::ignoreLimits && CapricaConfig::warningsToIgnore.count(warningNumber) == 0;
  }
  return CapricaConfig::treatWarningsAsErrors || CapricaConfig::warningsToHandleAsErrors.count(warningNumber);
}

bool CapricaError::isWarningEnabled(size_t warningNumber) {
  return CapricaConfig::warningsToIgnore.count(warningNumber) == 0 && CapricaConfig::warningsToHandleAsErrors.count(warningNumber) == 0;
}

}
