#pragma once

#include <string>

#include <boost/filesystem.hpp>

namespace caprica { namespace FSUtils {

boost::filesystem::path canonical(const boost::filesystem::path& path);
boost::filesystem::path naive_uncomplete(const boost::filesystem::path p, const boost::filesystem::path base);

}}
