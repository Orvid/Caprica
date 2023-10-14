#include "CapricaInputFile.h"
#include <common/CapricaConfig.h>
#include <common/CapricaInputFile.h>
#include <common/FSUtils.h>
#include <common/parser/PapyrusProject.h>
#include <filesystem>

namespace caprica {

std::filesystem::path IInputFile::resolved_relative() const {
  if (!resolved)
    return {};
  auto rel = absPath.lexically_relative(absBaseDir);
  if (rel == ".")
    return {};
  return rel;
}

std::filesystem::path IInputFile::resolved_absolute() const {
  if (!resolved)
    return {};
  return absPath;
}

std::filesystem::path IInputFile::resolved_absolute_basedir() const {
  if (!resolved)
    return {};
  return absBaseDir;
}

std::filesystem::path IInputFile::resolved_relative_parent() const {
  if (!resolved)
    return {};
  auto rel = resolved_relative();
  if (rel.empty())
    return {};
  return rel.parent_path();
}

std::filesystem::path IInputFile::find_import_dir(const std::filesystem::path& path) {
  for (auto& dir : conf::Papyrus::importDirectories)
    if (dirContains(path, dir.resolved_absolute()))
      return dir.resolved_absolute();
  return {};
}

bool IInputFile::dirContains(const std::filesystem::path& path, const std::filesystem::path& dir) {
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

IInputFile::IInputFile(const std::filesystem::path& _path, bool noRecurse, const std::filesystem::path& _cwd)
    : noRecurse(noRecurse),
      rawPath(std::move(_path)),
      cwd(_cwd.empty() ? std::filesystem::current_path() : std::move(FSUtils::canonicalFS(_cwd))) {
}

bool IInputFile::exists() const {
  if (!resolved)
    return false;
  return std::filesystem::exists(resolved_absolute());
}

bool IInputFile::isDir() const {
  return std::filesystem::is_directory(resolved_absolute());
}

bool InputFile::resolve() {
  auto normalPath = FSUtils::normalize(rawPath);
  auto str = normalPath.string();
  if (!normalPath.is_absolute()) {
    absPath = FSUtils::canonicalFS(cwd / normalPath);
    absBaseDir = cwd;
    if (std::filesystem::exists(absPath)) {
      resolved = true;
      return true;
    } else {
      return false;
    }
  } else {
    absPath = FSUtils::canonicalFS(normalPath);
    absBaseDir = find_import_dir(absPath);
    if (absBaseDir.empty()) {
      absBaseDir = absPath.parent_path();
      requiresPreParse = true;
    }
  }

  if (std::filesystem::exists(absPath)) {
    resolved = true;
    return true;
  }

  return false;
}

InputFile::InputFile(const std::filesystem::path& _path, bool noRecurse, const std::filesystem::path& _cwd)
    : IInputFile(_path, noRecurse, _cwd) {
  requiresPreParse = true; // we always require pre-parse for non-PCompiler-compatible input files
}

ImportDir::ImportDir(const std::filesystem::path& _path, bool noRecurse, const std::filesystem::path& _cwd)
    : IInputFile(_path, noRecurse, _cwd) {
  requiresPreParse = true; // we always require pre-parse for import dirs
  import = true;
  resolve(); // we resolve import dirs immediately
}

bool ImportDir::resolve() {
  if (!rawPath.is_absolute())
    absPath = FSUtils::canonicalFS(cwd / rawPath);
  else
    absPath = FSUtils::canonicalFS(rawPath);
  absBaseDir = absPath;
  if (std::filesystem::exists(absPath) && std::filesystem::is_directory(absPath)) {
    resolved = true;
    return true;
  }
  return false;
}

PCompInputFile::PCompInputFile(const std::filesystem::path& _path,
                               bool noRecurse,
                               bool isFolder,
                               const std::filesystem::path& _cwd)
    : IInputFile(_path, noRecurse, _cwd) {
  __isFolder = isFolder;
}

static constexpr char const curDir[3] = { '.', FSUtils::SEP, 0 };
static constexpr char const parent[4] = { '.', '.', FSUtils::SEP, 0 };

bool PCompInputFile::resolve() {
  std::filesystem::path normalPath = FSUtils::objectNameToPath(rawPath.string());
  if (!__isFolder && normalPath.extension().empty())
    normalPath.replace_extension(".psc");
  normalPath = FSUtils::normalize(normalPath);
  std::string str = normalPath.string();
  // special case for relative paths that contain parent/cwd refs
  if (!normalPath.is_absolute() && (str == "." || str == ".." || str.starts_with(curDir) || str.contains(parent))) {
    absPath = FSUtils::canonicalFS(cwd / normalPath);
    absBaseDir = cwd;

    if (!std::filesystem::exists(absPath))
      return false;
    resolved = true;
    return true;
  }

  // if this is a relative folder path, and the folder is in the cwd, use cwd as the base dir
  if (__isFolder && !normalPath.is_absolute() && dirContains(normalPath, cwd))
    absBaseDir = cwd;
  else
    absBaseDir = find_import_dir(normalPath);

  if (absBaseDir.empty())
    return false;

  if (!normalPath.is_absolute())
    absPath = FSUtils::canonicalFS(absBaseDir / normalPath);
  else
    absPath = FSUtils::canonicalFS(normalPath);

  if (!std::filesystem::exists(absPath))
    return false;
  if (__isFolder && !std::filesystem::is_directory(absPath))
    return false;

  resolved = true;
  return true;
}

} // namespace caprica
