#pragma once
#include <filesystem>
namespace caprica {
struct IInputFile;
struct IInputFile {
  std::filesystem::path resolved_relative() const;
  std::filesystem::path resolved_absolute() const;
  std::filesystem::path resolved_absolute_basedir() const;
  std::filesystem::path resolved_relative_parent() const;
  std::filesystem::path get_unresolved_path() const { return rawPath; }
  bool isRecursive() const { return !noRecurse; }
  bool isImport() const { return import; }
  bool isResolved() const { return resolved; }
  virtual bool exists() const;
  virtual bool isDir() const;
  virtual bool resolve() = 0;
  IInputFile(const std::filesystem::path& _path, bool noRecurse = true, const std::filesystem::path& _cwd = "");
  virtual ~IInputFile() = default;

protected:
  bool noRecurse = true;
  const std::filesystem::path rawPath;
  const std::filesystem::path cwd;
  std::filesystem::path absPath;
  std::filesystem::path absBaseDir; // import dir
  static std::filesystem::path find_import_dir(const std::filesystem::path& _path);
  static bool dirContains(const std::filesystem::path& _path, const std::filesystem::path& dir);

  bool resolved = false;
  bool import = false;
  bool requiresPreParse = false;
};

struct InputFile : public IInputFile {
  virtual bool resolve() override;
  InputFile(const std::filesystem::path& _path, bool noRecurse = true, const std::filesystem::path& _cwd = "");
  virtual ~InputFile() = default;
};

struct PCompInputFile : public IInputFile {
  PCompInputFile(const std::filesystem::path& _path,
                 bool noRecurse = true,
                 bool isFolder = false,
                 const std::filesystem::path& _cwd = "");
  virtual bool isDir() const override { return __isFolder; }
  virtual bool resolve() override;

private:
  bool __isFolder = false;
};

struct ImportDir : public IInputFile {
  ImportDir(const std::filesystem::path& _path, bool noRecurse = true, const std::filesystem::path& _cwd = "");
  virtual bool isDir() const override { return true; }
  virtual bool resolve() override;

private:
  std::filesystem::path resolvedPath;
};
} // namespace caprica
