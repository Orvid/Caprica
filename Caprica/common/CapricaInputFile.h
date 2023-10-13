#pragma once
#include <filesystem>
namespace caprica{
struct IInputFile;
struct IInputFile : std::enable_shared_from_this<IInputFile>{
  virtual std::filesystem::path resolved_relative() const = 0;
  virtual std::filesystem::path resolved_absolute() const = 0;
  virtual std::filesystem::path resolved_absolute_basedir() const = 0;
  std::filesystem::path get_unresolved_path() const { return rawPath; }
  bool isRecursive() const { return !noRecurse; }
  bool isImport() const { return import; }
  virtual bool exists() const = 0;
  virtual bool isDir() const = 0;
  virtual bool resolve() = 0;
  virtual bool isResolved() const { return resolved; }
  IInputFile(const std::filesystem::path& _path,
            bool noRecurse = true,
            const std::filesystem::path& _cwd = "");
  virtual ~IInputFile() = default;

  // static InputFile createNewStyle(const std::filesystem::path& path, bool noRecurse = true, const std::filesystem::path& cwd = std::filesystem::current_path());
  // static InputFile createOldStyle(const std::filesystem::path& path, bool noRecurse = true, const std::filesystem::path& cwd = std::filesystem::current_path(), bool isFolder = false);
protected:
  bool noRecurse = true;
  const std::filesystem::path rawPath;
  const std::filesystem::path cwd;
  bool resolved = false;
  bool import = false;
  bool requiresPreParse = false;
  static std::filesystem::path find_import_dir(const std::filesystem::path& _path);
  static bool dirContains(const std::filesystem::path& _path, const std::filesystem::path& dir);
};
struct InputFile : public IInputFile {
  virtual std::filesystem::path resolved_relative() const override;
  virtual std::filesystem::path resolved_absolute() const override;
  virtual std::filesystem::path resolved_absolute_basedir() const override;
  virtual bool exists() const override;
  virtual bool isDir() const override;
  virtual bool resolve() override;
  InputFile(const std::filesystem::path& _path,
            bool noRecurse = true,
            const std::filesystem::path& _cwd = "");
  virtual ~InputFile() = default;

  private:
    std::filesystem::path absPath;
    std::filesystem::path absBaseDir;
};

struct PCompInputFile : public IInputFile {
  PCompInputFile(const std::filesystem::path& _path,
             bool noRecurse = true, bool isFolder = false,
             const std::filesystem::path& _cwd = "");
  virtual std::filesystem::path resolved_relative() const override;
  virtual std::filesystem::path resolved_absolute() const override;
  virtual std::filesystem::path resolved_absolute_basedir() const override;
  virtual bool exists() const override;
  virtual bool isDir() const override { return __isFolder; }
  virtual bool resolve() override;
  private:
    bool __isFolder = false;
    std::filesystem::path absPath;
    std::filesystem::path absBaseDir;
};

struct ImportDir : public IInputFile {
  ImportDir(const std::filesystem::path& _path,
             bool noRecurse = true,
             const std::filesystem::path& _cwd = "");
  virtual std::filesystem::path resolved_relative() const override;
  virtual std::filesystem::path resolved_absolute() const override;
  virtual std::filesystem::path resolved_absolute_basedir() const override;
  virtual bool exists() const override;
  virtual bool isDir() const override { return true; }
  virtual bool resolve() override;
  private:
    std::filesystem::path resolvedPath;
};
} // namespace caprica