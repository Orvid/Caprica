#include <CapricaConfig.h>

namespace caprica { namespace CapricaConfig {

bool compileInParallel{ false };
bool enableLanguageExtensions{ true };
bool enableSpeculativeSyntax{ true };
bool enableDecompiledStructNameRefs{ true };
bool allowCompilerIdentifiers{ true };
bool enableOptimizations{ true };
bool enableCKOptimizations{ true };
bool emitDebugInfo{ true };
std::vector<std::string> importDirectories{
  "./",
};

}}