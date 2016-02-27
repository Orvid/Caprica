#include <common/CapricaConfig.h>

namespace caprica { namespace CapricaConfig {

// These should always be defaulted to false/empty, and their real
// default values set in the command line parsing.

bool allowCompilerIdentifiers{ false };
bool allowDecompiledStructNameRefs{ false };
bool asyncFileRead{ false };
bool asyncFileWrite{ false };
bool compileInParallel{ false };
bool debugControlFlowGraph{ false };
bool dumpPexAsm{ false };
bool enableCKOptimizations{ false };
bool enableLanguageExtensions{ false };
bool enableOptimizations{ false };
bool enableSpeculativeSyntax{ false };
bool emitDebugInfo{ false };
std::vector<std::string> importDirectories{ };
bool quietCompile{ false };
bool resolveSymlinks{ false };
bool treatWarningsAsErrors{ false };
CapricaUserFlagsDefinition userFlagsDefinition{ };
std::unordered_set<size_t> warningsToHandleAsErrors{ };
std::unordered_set<size_t> warningsToIgnore{ };

}}
