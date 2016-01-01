#include <CapricaConfig.h>

namespace caprica { namespace CapricaConfig {

bool enableLanguageExtensions{ false };
bool enableSpeculativeSyntax{ true };
bool enableOptimizations{ true };
bool enableCKOptimizations{ true };
std::vector<std::string> importDirectories{ };

}}