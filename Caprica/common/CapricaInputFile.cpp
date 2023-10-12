#include <filesystem>
#include <common/CapricaInputFile.h>
#include <common/FSUtils.h>
#include <common/parser/PapyrusProject.h>
#include <common/CapricaConfig.h>

namespace caprica{

  std::filesystem::path InputFile::resolved_relative() const {
    for (auto& dir : conf::Papyrus::importDirectories)
      if (dirContains(dir.resolved_absolute()))
        return get_relative_path(dir.resolved_absolute());
    return {}; // not found
  }

  std::filesystem::path InputFile::resolved_absolute() const {
    // find the file among the import directories
    for (auto& dir : conf::Papyrus::importDirectories)
      if (dirContains(dir.resolved_absolute()))
        return get_absolute_path(dir.resolved_absolute());
    return {}; // not found
  }

  std::filesystem::path InputFile::resolved_absolute_basedir() const {
    // find the file among the import directories
    for (auto& dir : conf::Papyrus::importDirectories)
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
    if (str == "." || str == ".." || str.starts_with(".\\") || str.starts_with("./") || str.contains("..\\") ||
        str.contains("../")) {
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

} // namespace caprica