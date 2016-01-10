#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <string>

#include <boost/algorithm/string/case_conv.hpp>

namespace caprica {

struct CaselessStringComparer final : public std::function<bool(std::string, std::string)>
{
  bool operator()(const std::string &lhs, const std::string &rhs) const {
    return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
  }
};

struct CaselessStringHasher final : public std::function<size_t(std::string)>
{
  size_t operator()(const std::string& k) const {
    std::string lowStr = k;
    boost::algorithm::to_lower(lowStr);
    return std::hash<std::string>()(lowStr);
  }
};

struct CaselessStringEqual final : public std::function<bool(std::string, std::string)>
{
  bool operator()(const std::string &lhs, const std::string &rhs) const {
    return _stricmp(lhs.c_str(), rhs.c_str()) == 0;
  }
};

}
