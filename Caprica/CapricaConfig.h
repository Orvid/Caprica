#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace caprica { namespace CapricaConfig {

// Enable Caprica extensions to the Papyrus language.
extern bool enableLanguageExtensions;
// Enable the speculated form of the new functionality available
// to Papyrus in FO4.
extern bool enableSpeculativeSyntax;
// Enable optimizations normally enabled by the -optimize switch to the
// CK compiler.
extern bool enableOptimizations;
// Enable optimizations that are done regardless of if the -optimize
// switch is passed to the CK compiler.
extern bool enableCKOptimizations;
// The directories to search in for imported types and
// unknown types.
extern std::vector<std::string> importDirectories;

}}