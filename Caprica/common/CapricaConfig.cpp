#include "CapricaConfig.h"
#include <common/CapricaConfig.h>
#include <common/FSUtils.h>
#include <filesystem>
namespace caprica { namespace conf {

// These should always be defaulted to false/empty, and their real
// default values set in the command line parsing.

namespace General {
  bool compileInParallel{ false };
  bool quietCompile{ false };
  bool recursive { false };
  std::filesystem::path outputDirectory;
  bool anonymizeOutput;
  std::vector<InputFile> inputFiles;
  }

  namespace PCompiler {
  // pCompiler compatibility mode.
  bool pCompilerCompatibilityMode{false};
  bool all{false};
  bool norecurse{false};
}

namespace CodeGeneration {
  bool disableBetaCode{ false };
  bool disableDebugCode{ false };
  bool enableCKOptimizations{ false };
  bool enableOptimizations{ false };
  bool emitDebugInfo{ false };
}

namespace Debug {
  bool debugControlFlowGraph{ false };
  bool dumpPexAsm{ false };
}

namespace EngineLimits {
  bool ignoreLimits{ false };
  size_t maxArrayLength{ 0 };
  size_t maxFunctionsInEmptyStatePerObject{ 0 };
  size_t maxFunctionsPerState{ 0 };
  size_t maxGuardsPerObject{ 0 };
  size_t maxInitialValuesPerObject{ 0 };
  size_t maxNamedStatesPerObject{ 0 };
  size_t maxParametersPerFunction{ 0 };
  size_t maxPropertiesPerObject{ 0 };
  size_t maxStaticFunctionsPerObject{ 0 };
  size_t maxUserFlags{ 0 };
  size_t maxVariablesPerObject{ 0 };
}

namespace Papyrus {
GameID game { GameID::UNKNOWN };
bool allowCompilerIdentifiers { false };
bool allowDecompiledStructNameRefs{ false };
  bool allowNegativeLiteralAsBinaryOp{ false };
  bool enableLanguageExtensions{ false };
  bool ignorePropertyNameLocalConflicts{ false };
  bool allowImplicitNoneCastsToAnyType{ false };
  std::vector<ImportFile> importDirectories {};
  CapricaUserFlagsDefinition userFlagsDefinition{ };
}

namespace Skyrim {
  bool skyrimAllowUnknownEventsOnNonNativeClass{ true };
  bool skyrimAllowObjectVariableShadowingParentProperty{ true };
  bool skyrimAllowLocalVariableShadowingParentProperty{ true };
  bool skyrimAllowLocalUseBeforeDeclaration{ true };
  bool skyrimAllowAssigningVoidMethodCallResult{ true };
}

namespace Performance {
  bool asyncFileRead{ false };
  bool asyncFileWrite{ false };
  bool dumpTiming{ false };
  bool performanceTestMode{ false };
  bool resolveSymlinks{ false };
}

namespace Warnings {
  bool disableAllWarnings{ false };
  bool treatWarningsAsErrors{ false };
  std::unordered_set<size_t> warningsToHandleAsErrors{ };
  std::unordered_set<size_t> warningsToIgnore{ };
  std::unordered_set<size_t> warningsToEnable{ };
}

  std::filesystem::path InputFile::resolved_relative() const {
    for (auto& dir : Papyrus::importDirectories)
      if (dirContains(dir.resolved_absolute()))
        return get_relative_path(dir.resolved_absolute());
    return {}; // not found
  }

  std::filesystem::path InputFile::resolved_absolute() const {
    // find the file among the import directories
    for (auto& dir : Papyrus::importDirectories)
      if (dirContains(dir.resolved_absolute()))
        return get_absolute_path(dir.resolved_absolute());
    return {}; // not found
  }

  std::filesystem::path InputFile::resolved_absolute_basedir() const {
    // find the file among the import directories
    for (auto& dir : Papyrus::importDirectories)
      if (dirContains(dir.resolved_absolute()))
        return dir.resolved_absolute();
    return {}; // not found
  }

  std::filesystem::path InputFile::get_absolute_path(const std::filesystem::path& absDir) const {
    if (path.is_absolute())
      return FSUtils::canonicalFS(path);
    else
      return FSUtils::canonicalFS((absDir / path));
  }

  std::filesystem::path InputFile::get_relative_path(const std::filesystem::path& absDir) const {
    std::filesystem::path cpath;
    if (path.is_absolute())
      cpath = FSUtils::canonicalFS(path);
    else
      cpath = FSUtils::canonicalFS((absDir / path));
    return cpath.lexically_relative(absDir);
  }

  bool InputFile::dirContains(const std::filesystem::path& dir) const {

    if (path.is_absolute()) {
      // check if the path is contained in the import directory
      auto rel = path.lexically_relative(dir).string();
      if (!rel.empty() && !rel.starts_with(".."))
        return true;
    } else {
      if (std::filesystem::exists(dir / path))
        return true;
    }
    return false;
  }
  InputFile::InputFile(const std::filesystem::path& _path, bool noRecurse, const std::filesystem::path& _cwd)
      : noRecurse(noRecurse),
        path(std::move(_path)),
        cwd(_cwd.empty() ? std::filesystem::current_path() : std::move(_cwd)) {
    path = path.make_preferred();
    // special handler; this points to something relative to the cwd, not an object path to be resolved
    auto str = path.string();
    if (str.starts_with(".\\") || str.starts_with("./") || str.contains("..\\") || str.contains("../")) {
      if (!path.is_absolute())
        path = cwd / path;
      path = FSUtils::canonicalFS(path);
    }
  }

  bool InputFile::exists() const {
    return std::filesystem::exists(resolved_absolute());
  }

  ImportFile::ImportFile(const std::filesystem::path& _path, bool noRecurse, const std::filesystem::path& _cwd)
      : InputFile(_path, noRecurse, _cwd) {
    import = true;
    // make the import path absolute
    if (!path.is_absolute())
      path = cwd / path;
    path = FSUtils::canonicalFS(path);
  }

  std::filesystem::path ImportFile::resolved_relative() const {
    return {};
  }

  std::filesystem::path ImportFile::resolved_absolute() const {
    return path; // we always return the absolute path for imports
  }

  std::filesystem::path ImportFile::resolved_absolute_basedir() const {
    return path;
  }

  }}
