#include <common/FSUtils.h>

#include <fcntl.h>
#include <io.h>

#include <cstring>
#include <filesystem>
#include <future>
#include <sstream>

#include <common/CapricaConfig.h>
#include <common/CapricaReportingContext.h>
#include <common/CaselessStringComparer.h>

namespace caprica { namespace FSUtils {

std::string_view basenameAsRef(std::string_view file) {
  auto pos = file.find_last_of("\\/");
  auto fil = file;
  if (pos != std::string_view::npos)
    fil = file.substr(pos + 1);
  auto pos2 = fil.rfind('.');
  if (pos2 != std::string_view::npos)
    return fil.substr(0, pos2);
  return fil;
}

std::string_view extensionAsRef(std::string_view file) {
  auto pos = file.rfind('.');
  if (pos != std::string_view::npos)
    return file.substr(pos);
  return "";
}

std::string_view filenameAsRef(std::string_view file) {
  auto pos = file.find_last_of("\\/");
  if (pos != std::string_view::npos)
    return file.substr(pos + 1);
  return file;
}

std::string_view parentPathAsRef(std::string_view file) {
  auto pos = file.find_last_of("\\/");
  if (pos != std::string_view::npos)
    return file.substr(0, pos);
  return file;
}

bool shouldShortCircuit(const std::string& path) {
  if (path.size() > 3 && path[1] == ':') {
    // Shortcircuit for already canon paths.
    if (path.find("/") == std::string::npos && path.find("..") == std::string::npos &&
        path.find("\\.\\") == std::string::npos && path.find("\\\\") == std::string::npos) {
      return true;
    }
  }
  return false;
}

// Borrowed and modified from http://stackoverflow.com/a/1750710/776797
std::string canonical(const std::string& path) {
  return canonicalFS(path).string();
}

std::filesystem::path canonicalFS(const std::filesystem::path& path) {
  if (shouldShortCircuit(path.string()))
    return path;
  auto absPath = std::filesystem::absolute(path);
  if (conf::Performance::resolveSymlinks) {
    return std::filesystem::canonical(absPath).make_preferred().string();
  } else {
    std::filesystem::path result;
    for (auto it = absPath.begin(); it != absPath.end(); ++it) {
      if (!wcscmp(it->c_str(), L"..")) {
        // /a/b/../.. is not /a/b/.. under most circumstances
        // We can end up with ..s in our result because of symbolic links
        if (!wcscmp(result.filename().c_str(), L".."))
          result /= *it;
        // Otherwise it should be safe to resolve the parent
        else
          result = result.parent_path();
      } else if (!wcscmp(it->c_str(), L".")) {
        // Ignore
      } else {
        // Just cat other path entries
        result /= *it;
      }
    }
    return result.make_preferred();
  }
}

}}
