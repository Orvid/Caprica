#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

#include <common/CapricaUserFlagsDefinition.h>

namespace caprica { namespace CapricaConfig {

// If true, allow identifiers to be prefixed with '::', which are normally
// reserved for compiler identifiers.
extern bool allowCompilerIdentifiers;
// Allow the parsing of references to structs as presented by
// Champollion, where the script name is prepended to the struct
// name and separated by a '#'.
extern bool allowDecompiledStructNameRefs;
// If true, read files asyncronously in an attempt to pre-emptively
// read them from disk. This results in worse performance on HDDs,
// but better performance on SSDs, as they are actually able to read
// multiple files at once.
extern bool asyncFileRead;
// If true, write files to disk on background threads, allowing
// the main compile threads to keep working while waiting for the
// disk to catch up.
extern bool asyncFileWrite;
// If true, when compiling multiple files, do so
// in multiple threads.
extern bool compileInParallel;
// If true, output the control flow graph of every function in the
// files being compiled to stdout.
extern bool debugControlFlowGraph;
// If true, dump the Asm representation of the Pex file generated
// for the Papyrus scripts being compiled.
extern bool dumpPexAsm;
// Enable optimizations that are done regardless of if the -optimize
// switch is passed to the CK compiler.
extern bool enableCKOptimizations;
// Enable Caprica extensions to the Papyrus language.
extern bool enableLanguageExtensions;
// Enable optimizations normally enabled by the -optimize switch to the
// CK compiler.
extern bool enableOptimizations;
// If true, emit debug info for the papyrus script.
extern bool emitDebugInfo;
// The directories to search in for imported types and
// unknown types.
extern std::vector<std::string> importDirectories;
// If true, we pause and wait for all files to be read in before
// compiling them, and we also don't write them out to disk.
// This is done to increase the consistency of the test runs.
extern bool performanceTestMode;
// If true, only report failures, not progress.
extern bool quietCompile;
// If true, resolve symlinks while building canonical
// paths.
extern bool resolveSymlinks;
// If true, treat warnings as errors.
extern bool treatWarningsAsErrors;
// The user flags definition.
extern CapricaUserFlagsDefinition userFlagsDefinition;
// The set of warnings to treat as errors.
extern std::unordered_set<size_t> warningsToHandleAsErrors;
// The set of warnings to ignore.
extern std::unordered_set<size_t> warningsToIgnore;

// Limitations of the game engine, not of Caprica.
struct EngineLimits final
{
  // If true, warn when the limits are exceeded, but allow compilation to continue anyways.
  static bool ignoreLimits;
  // The maximum length of an array. 0 means no limit.
  static size_t maxArrayLength;
  // The maximum number of functions in the empty state in a single object. 0 means no limit.
  static size_t maxFunctionsInEmptyStatePerObject;
  // The maximum number of functions in a single state. 0 means no limit.
  static size_t maxFunctionsPerState;
  // The maximum number of variables in a single object that can have initial values. 0 means no limit.
  static size_t maxInitialValuesPerObject;
  // The maximum number of named states in a single object. 0 means no limit.
  static size_t maxNamedStatesPerObject;
  // The maximum number of parameters to a single function. 0 means no limit.
  static size_t maxParametersPerFunction;
  // The maximum number of properties in a single object. 0 means no limit.
  static size_t maxPropertiesPerObject;
  // The maximum number of global functions allowed in a single object. 0 means no limit.
  static size_t maxStaticFunctionsPerObject;
  // The maximum number of distinct user flags allowed. Composite flags do not count toward this limit.
  static size_t maxUserFlags;
  // The maximum number of variables in a single object. 0 means no limit.
  static size_t maxVariablesPerObject;
};

}}
