#include <filesystem>
#include <common/CapricaInputFile.h>
#include <common/FSUtils.h>
#include <common/parser/PapyrusProject.h>
#include <common/CapricaConfig.h>
#include "CapricaInputFile.h"

namespace caprica{

  std::filesystem::path InputFile::resolved_relative() const {
    if (!resolved) return {};
    return absPath.lexically_relative(absBaseDir);
  }

  std::filesystem::path InputFile::resolved_absolute() const {
    if (!resolved) return {};
    return absPath; 
  }

  std::filesystem::path InputFile::resolved_absolute_basedir() const {
    if (!resolved) return {};
    return absBaseDir;
  }

  std::filesystem::path IInputFile::find_import_dir(const std::filesystem::path& path) {
    for (auto& dir : conf::Papyrus::importDirectories) {
      if (dirContains(path, dir.resolved_absolute())) {
        return dir.resolved_absolute();
      }
    }
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

  bool InputFile::exists() const {
    return std::filesystem::exists(resolved_absolute());
  }

  bool InputFile::isDir() const {
    return std::filesystem::is_directory(resolved_absolute());
  }

  bool InputFile::resolve() {
    auto normalPath = rawPath.lexically_normal();
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

  InputFile::InputFile(const std::filesystem::path& _path, bool noRecurse, const std::filesystem::path& _cwd): IInputFile(_path, noRecurse, _cwd) {
  }

  ImportDir::ImportDir(const std::filesystem::path& _path, bool noRecurse, const std::filesystem::path& _cwd)
      : IInputFile(_path, noRecurse, _cwd) {
    import = true;
    resolve(); // we resolve import dirs immediately
  }

  std::filesystem::path ImportDir::resolved_relative() const {
    return {};
  }

  std::filesystem::path ImportDir::resolved_absolute() const {
    return resolvedPath; // we always return the absolute path for imports
  }

  std::filesystem::path ImportDir::resolved_absolute_basedir() const {
    return resolvedPath;
  }

  bool ImportDir::exists() const {
    return std::filesystem::exists(resolvedPath);
  }

  bool ImportDir::resolve() {
    if (!rawPath.is_absolute())
      resolvedPath = FSUtils::canonicalFS(cwd / rawPath);
    else
      resolvedPath = FSUtils::canonicalFS(rawPath);

    if (std::filesystem::exists(resolvedPath) && std::filesystem::is_directory(resolvedPath)) {
      resolved = true;
      return true;
    }
    return false;
  }

  PCompInputFile::PCompInputFile(const std::filesystem::path& _path,
                                          bool noRecurse,
                                          bool isFolder,
                                          const std::filesystem::path& _cwd): IInputFile(_path, noRecurse, _cwd) {
    __isFolder = isFolder;
  }

  std::filesystem::path PCompInputFile::resolved_relative() const {
    if (!resolved) return {};
    return absPath.lexically_relative(absBaseDir);
  }

  std::filesystem::path PCompInputFile::resolved_absolute() const {
    if (!resolved) return {};
    return absPath;
  }

  std::filesystem::path PCompInputFile::resolved_absolute_basedir() const {
    if (!resolved) return {};
    return absBaseDir;
  }

  bool PCompInputFile::exists() const {
    return std::filesystem::exists(resolved_absolute());
  }

  bool PCompInputFile::resolve() {
    auto normalPath = rawPath;
    auto str = normalPath.string();
    // replace all ':' with preferred seperator
    std::replace(str.begin(), str.end(), ':', FSUtils::SEP);
    normalPath = str;
    if (!__isFolder && normalPath.extension().empty())
      normalPath.replace_extension(".psc");
    normalPath = normalPath.lexically_normal();
    std::string sep = {FSUtils::SEP};
    str = normalPath.string();


    // special case for relative paths that contain parent/cwd refs
    if (!normalPath.is_absolute() && (str == "." || str == ".." || str.starts_with("." + sep) || str.contains(".." + sep))) {
      absPath = FSUtils::canonicalFS(cwd / normalPath);
      absBaseDir = cwd;
      
      if (!std::filesystem::exists(absPath))
        return false;
      resolved = true;
      return true;
    }

    // if this is a relative folder path, and the folder is in the cwd, use cwd as the base dir
    if (__isFolder && !normalPath.is_absolute() && dirContains(normalPath, cwd)) {
      absBaseDir = cwd;
    } else {
      absBaseDir = find_import_dir(normalPath); 
    }

    if (absBaseDir.empty()) {
      return false;
    }

    if (!normalPath.is_absolute()) {
      absPath = FSUtils::canonicalFS(absBaseDir / normalPath);
    } else {
      absPath = FSUtils::canonicalFS(normalPath);
    }
  
    if (std::filesystem::exists(absPath)) {
      resolved = true;
      return true;
    }
    return false;
  }

} // namespace caprica
