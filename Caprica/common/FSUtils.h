#pragma once

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
  static boost::string_ref cachedReadFull(const std::string& filename);
};

boost::string_ref basenameAsRef(boost::string_ref file);
boost::string_ref extensionAsRef(boost::string_ref file);
boost::string_ref filenameAsRef(boost::string_ref file);
boost::string_ref parentPathAsRef(boost::string_ref file);

void async_write(const std::string& filename, std::string&& value);
bool exists(const std::string& path);
void pushKnownExists(const std::string& path);
void pushKnownInDirectory(const std::string& directory, caseless_unordered_set<std::string>&& files);
std::string canonical(const std::string& path);

}}
