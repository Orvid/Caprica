#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace caprica { namespace CapricaConfig {

// If true, when compiling multiple files, do so
// in multiple threads.
extern bool compileInParallel;
// Enable Caprica extensions to the Papyrus language.
extern bool enableLanguageExtensions;
// Enable the speculated form of the new functionality available
// to Papyrus in FO4.
extern bool enableSpeculativeSyntax;
// Enable the parsing of references to structs as presented by
// Champollion, where the script name is prepended to the struct
// name and separated by a '#'.
extern bool enableDecompiledStructNameRefs;
// If true, allow identifiers to be prefixed with '::', which are normally
// reserved for compiler identifiers.
extern bool allowCompilerIdentifiers;
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