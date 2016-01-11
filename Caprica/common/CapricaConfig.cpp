#include <common/CapricaConfig.h>

namespace caprica { namespace CapricaConfig {

bool allowCompilerIdentifiers{ true };
bool allowDecompiledStructNameRefs{ true };
bool compileInParallel{ false };
bool dumpPexAsm{ false };
bool enableCKOptimizations{ true };
bool enableLanguageExtensions{ true };
bool enableOptimizations{ true };
bool enableSpeculativeSyntax{ true };
bool emitDebugInfo{ true };
std::vector<std::string> importDirectories{ };
bool quietCompile{ false };
bool treatWarningsAsErrors{ false };
CapricaUserFlagsDefinition userFlagsDefinition{ };
std::unordered_set<size_t> warningsToHandleAsErrors{ };
std::unordered_set<size_t> warningsToIgnore{ };

}}
