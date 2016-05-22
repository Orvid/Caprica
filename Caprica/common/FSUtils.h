#pragma once

#include <array>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/utility/string_ref.hpp>

#include <common/CaselessStringComparer.h>

namespace caprica { namespace FSUtils {

struct Cache abstract
{
  explicit Cache() = delete;
  Cache(const Cache&) = delete;
  ~Cache() = delete;

  static void waitForAll();
  static void push_need(const std::string& filename, size_t filesize = 0);
  static std::string cachedReadFull(const std::string& filename);
};

boost::string_ref basenameAsRef(boost::string_ref file);
boost::string_ref extensionAsRef(boost::string_ref file);
boost::string_ref filenameAsRef(boost::string_ref file);

void async_write(const std::string& filename, std::string&& value);
const char* filenameAsRef(const std::string& file);
bool exists(const std::string& path);
std::array<bool, 3> multiExistsInDir(const std::string& dir, std::array<std::string, 3>&& filenames);
void pushKnownExists(const std::string& path);
void pushKnownInDirectory(const std::string& directory, caseless_unordered_set<std::string>&& files);
boost::filesystem::path canonical(const boost::filesystem::path& path);

}}
