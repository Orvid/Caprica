#include <CapricaConfig.h>

namespace caprica { namespace CapricaConfig {

bool allowCompilerIdentifiers{ true };
bool compileInParallel{ false };
bool dumpPexAsm{ false };
bool enableCKOptimizations{ true };
bool enableDecompiledStructNameRefs{ true };
bool enableLanguageExtensions{ true };
bool enableOptimizations{ true };
bool enableSpeculativeSyntax{ true };
bool emitDebugInfo{ true };
std::vector<std::string> importDirectories{
  "./",
};

}}