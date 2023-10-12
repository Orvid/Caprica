#pragma once
#include <filesystem>
namespace caprica{

struct InputFile {
  virtual std::filesystem::path resolved_relative() const;
  virtual std::filesystem::path resolved_absolute() const;
  virtual std::filesystem::path resolved_absolute_basedir() const;
  std::filesystem::path get_unresolved_path() const { return path; }
  bool isRecursive() const { return !noRecurse; }
  bool isImport() const { return import; }
  bool exists() const;
  InputFile(const std::filesystem::path& _path,
            bool noRecurse = true,
            const std::filesystem::path& _cwd = std::filesystem::current_path());

protected:
  bool noRecurse = true;
  std::filesystem::path path;
  std::filesystem::path cwd;
  bool resolved = false;
  bool import = false;
  virtual std::filesystem::path get_absolute_path(const std::filesystem::path& absDir) const;
  virtual std::filesystem::path get_relative_path(const std::filesystem::path& dir) const;
  bool dirContains(const std::filesystem::path&  dir) const;
};

struct ImportFile : public InputFile {
  ImportFile(const std::filesystem::path& _path,
             bool noRecurse = true,
             const std::filesystem::path& _cwd = std::filesystem::current_path());
  virtual std::filesystem::path resolved_relative() const override;
  virtual std::filesystem::path resolved_absolute() const override;
  virtual std::filesystem::path resolved_absolute_basedir() const override;
};
} // namespace caprica