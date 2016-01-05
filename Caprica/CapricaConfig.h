#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace caprica { namespace CapricaConfig {

// If true, allow identifiers to be prefixed with '::', which are normally
// reserved for compiler identifiers.
extern bool allowCompilerIdentifiers;
// If true, when compiling multiple files, do so
// in multiple threads.
extern bool compileInParallel;
// If true, dump the Asm representation of the Pex file generated
// for the Papyrus scripts being compiled.
extern bool dumpPexAsm;
// Enable optimizations that are done regardless of if the -optimize
// switch is passed to the CK compiler.
extern bool enableCKOptimizations;
// Enable the parsing of references to structs as presented by
// Champollion, where the script name is prepended to the struct
// name and separated by a '#'.
extern bool enableDecompiledStructNameRefs;
// Enable Caprica extensions to the Papyrus language.
extern bool enableLanguageExtensions;
// Enable optimizations normally enabled by the -optimize switch to the
// CK compiler.
extern bool enableOptimizations;
// Enable the speculated form of the new functionality available
// to Papyrus in FO4.
extern bool enableSpeculativeSyntax;
// If true, emit debug info for the papyrus script.
extern bool emitDebugInfo;
// The directories to search in for imported types and
// unknown types.
extern std::vector<std::string> importDirectories;

}}