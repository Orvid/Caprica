#include <common/CapricaError.h>

#include <common/CapricaConfig.h>

namespace caprica {

size_t CapricaError::warningCount{ 0 };
size_t CapricaError::errorCount{ 0 };

void CapricaError::exitIfErrors() {
  if (errorCount > 0) {
    std::cerr << "Compilation failed, " << warningCount << " warnings and " << errorCount << " errors were encountered." << std::endl;
    throw std::runtime_error("");
  }
}

bool CapricaError::isWarningEnabled(size_t warningNumber) {
  return CapricaConfig::warningsToIgnore.count(warningNumber) == 0 && CapricaConfig::warningsToHandleAsErrors.count(warningNumber) == 0;
}

}
