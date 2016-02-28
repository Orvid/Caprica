#pragma once

#include <string>

#include <boost/filesystem.hpp>

namespace caprica { namespace FSUtils {

struct Cache abstract
{
  explicit Cache() = delete;
  Cache(const Cache&) = delete;
  ~Cache() = delete;

  static void waitForAll();
  static void push_need(const std::string& filename);
  static std::string cachedReadFull(const std::string& filename);

private:
  static std::string readFile(const std::string& filename);
};

void async_write(const std::string& filename, const std::string& value);
boost::filesystem::path canonical(const boost::filesystem::path& path);
boost::filesystem::path naive_uncomplete(const boost::filesystem::path p, const boost::filesystem::path base);

}}
