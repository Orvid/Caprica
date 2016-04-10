#include <common/CapricaConfig.h>

namespace caprica { namespace CapricaConfig {

// These should always be defaulted to false/empty, and their real
// default values set in the command line parsing.

bool allowCompilerIdentifiers{ false };
bool allowDecompiledStructNameRefs{ false };
bool allowNegativeLiteralAsBinaryOp{ false };
bool asyncFileRead{ false };
bool asyncFileWrite{ false };
bool compileInParallel{ false };
bool debugControlFlowGraph{ false };
bool dumpPexAsm{ false };
bool enableCKOptimizations{ false };
bool enableLanguageExtensions{ false };
bool enableOptimizations{ false };
bool emitDebugInfo{ false };
std::vector<std::string> importDirectories{ };
bool performanceTestMode{ false };
bool quietCompile{ false };
bool resolveSymlinks{ false };
bool treatWarningsAsErrors{ false };
CapricaUserFlagsDefinition userFlagsDefinition{ };
std::unordered_set<size_t> warningsToHandleAsErrors{ };
std::unordered_set<size_t> warningsToIgnore{ };

bool EngineLimits::ignoreLimits{ false };
size_t EngineLimits::maxArrayLength{ 0 };
size_t EngineLimits::maxFunctionsInEmptyStatePerObject{ 0 };
size_t EngineLimits::maxFunctionsPerState{ 0 };
size_t EngineLimits::maxInitialValuesPerObject{ 0 };
size_t EngineLimits::maxNamedStatesPerObject{ 0 };
size_t EngineLimits::maxParametersPerFunction{ 0 };
size_t EngineLimits::maxPropertiesPerObject{ 0 };
size_t EngineLimits::maxStaticFunctionsPerObject{ 0 };
size_t EngineLimits::maxUserFlags{ 0 };
size_t EngineLimits::maxVariablesPerObject{ 0 };

}}
