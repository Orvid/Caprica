#pragma once

#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <boost/utility/string_ref.hpp>

namespace caprica {

void identifierToLower(std::string& str);

bool caselessEq(boost::string_ref a, boost::string_ref b);
bool pathEq(boost::string_ref a, boost::string_ref b);

bool idEq(const char* a, const char* b);
bool idEq(const char* a, const std::string& b);
bool idEq(const std::string& a, const char* b);
__declspec(noinline)
bool idEq(const std::string& a, const std::string& b);
__declspec(noinline)
bool idEq(boost::string_ref a, boost::string_ref b);

struct CaselessStringHasher final : public std::function<size_t(const std::string&)>
{
  size_t operator()(const char* k) const = delete;
  size_t operator()(const std::string& k) const {
    return doCaselessHash(k.c_str(), k.size());
  }

private:
  friend struct CaselessPathHasher;
  __declspec(noinline)
  static size_t doCaselessHash(const char* k, size_t len);
};

struct CaselessStringEqual final : public std::function<bool(const std::string&, const std::string&)>
{
  bool operator()(const char* lhs, const char* rhs) const = delete;
  bool operator()(const std::string& lhs, const std::string& rhs) const {
    return caselessEq(lhs, rhs);
  }
};

struct CaselessPathHasher final : public std::function<size_t(const std::string&)>
{
  size_t operator()(const std::string& k) const {
    return doPathHash(k.c_str(), k.size());
  }

private:
  __declspec(noinline)
  static size_t doPathHash(const char* k, size_t len);
};

struct CaselessPathEqual final : public std::function<bool(const std::string&, const std::string&)>
{
  bool operator()(const std::string& lhs, const std::string& rhs) const {
    return pathEq(lhs, rhs);
  }
};

struct CaselessIdentifierHasher final : public std::function<size_t(const std::string&)>
{
  size_t operator()(const char* k) const = delete;
  __declspec(noinline)
  size_t operator()(const std::string& k) const;
  __declspec(noinline)
  size_t operator()(boost::string_ref k) const;
};

struct CaselessIdentifierEqual final : public std::function<bool(const std::string&, const std::string&)>
{
  bool operator()(const char* lhs, const char* rhs) const = delete;
  bool operator()(const std::string& lhs, const std::string& rhs) const {
    return idEq(lhs, rhs);
  }
  bool operator()(boost::string_ref lhs, boost::string_ref rhs) const {
    return idEq(lhs, rhs);
  }
};

// These aren't as restricted as identifiers, but must be in the base-ascii
// range, and must not contain control characters.
using caseless_unordered_set = std::unordered_set<std::string, CaselessStringHasher, CaselessStringEqual>;

// The path collections are for UTF-8 encoded strings, and when comparing two,
// they are likely to have a long prefix in common.
using caseless_unordered_path_set = std::unordered_set<std::string, CaselessPathHasher, CaselessPathEqual>;
template<typename V>
using caseless_unordered_path_map = typename std::unordered_map<std::string, V, CaselessPathHasher, CaselessPathEqual>;

using caseless_unordered_identifier_set = std::unordered_set<std::string, CaselessIdentifierHasher, CaselessIdentifierEqual>;
using caseless_unordered_identifier_ref_set = std::unordered_set<boost::string_ref, CaselessIdentifierHasher, CaselessIdentifierEqual>;
template<typename V>
using caseless_unordered_identifier_map = typename std::unordered_map<std::string, V, CaselessIdentifierHasher, CaselessIdentifierEqual>;
template<typename V>
using caseless_unordered_identifier_ref_map = typename std::unordered_map<boost::string_ref, V, CaselessIdentifierHasher, CaselessIdentifierEqual>;

}
