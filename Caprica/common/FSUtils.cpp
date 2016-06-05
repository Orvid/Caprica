#include <common/FSUtils.h>

#include <io.h>
#include <fcntl.h>

#include <cstring>
#include <future>
#include <sstream>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <common/CapricaConfig.h>
#include <common/CapricaReportingContext.h>
#include <common/CaselessStringComparer.h>
#include <common/Concurrency.h>

namespace caprica { namespace FSUtils {

boost::string_ref basenameAsRef(boost::string_ref file) {
  auto pos = file.find_last_of("\\/");
  if (pos != boost::string_ref::npos)
    file = file.substr(pos + 1);
  auto pos2 = file.rfind('.');
  if (pos2 != boost::string_ref::npos)
    return file.substr(0, pos2);
  return file;
}

boost::string_ref extensionAsRef(boost::string_ref file) {
  auto pos = file.rfind('.');
  if (pos != boost::string_ref::npos)
    return file.substr(pos);
  return "";
}

boost::string_ref filenameAsRef(boost::string_ref file) {
  auto pos = file.find_last_of("\\/");
  if (pos != boost::string_ref::npos)
    return file.substr(pos + 1);
  return file;
}

boost::string_ref parentPathAsRef(boost::string_ref file) {
  auto pos = file.find_last_of("\\/");
  if (pos != boost::string_ref::npos)
    file = file.substr(0, pos);
  return file;
}

// Borrowed and modified from http://stackoverflow.com/a/1750710/776797
std::string canonical(const std::string& path) {
  if (path.size() > 3 && path[1] == ':') {
    // Shortcircuit for already canon paths.
    if (path.find("/") == std::string::npos &&
        path.find("..") == std::string::npos &&
        path.find("\\.\\") == std::string::npos &&
        path.find("\\\\") == std::string::npos) {
      return path;
    }
  }
  static boost::filesystem::path currentDirectory = boost::filesystem::current_path();
  auto absPath = boost::filesystem::absolute(path, currentDirectory);
  if (conf::Performance::resolveSymlinks) {
    return boost::filesystem::canonical(absPath).make_preferred().string();
  } else {
    boost::filesystem::path result;
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
    return result.make_preferred().string();
  }
}

}}
