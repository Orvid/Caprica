#pragma once

#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace caprica {

struct CaselessStringComparer final : public std::function<bool(std::string, std::string)>
{
  bool operator()(const char* const lhs, const char* const rhs) const {
    return _stricmp(lhs, rhs) < 0;
  }

  bool operator()(const std::string &lhs, const std::string &rhs) const {
    return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
  }
};

struct CaselessStringHasher final : public std::function<size_t(std::string)>
{
  size_t operator()(const char* k) const {
    return doCaselessHash(k, strlen(k));
  }
  size_t operator()(const std::string& k) const {
    return doCaselessHash(k.c_str(), k.size());
  }

private:
  static size_t doCaselessHash(const char* k, size_t len);
};

struct CaselessIdentifierHasher final : public std::function<size_t(std::string)>
{
  size_t operator()(const char* k) const {
    return doIdentifierHash(k, strlen(k));
  }
  size_t operator()(const std::string& k) const {
    return doIdentifierHash(k.c_str(), k.size());
  }
private:
  static size_t doIdentifierHash(const char* k, size_t len);
};

//using CaselessStringHasher = CaselessIdentifierHasher;

struct CaselessStringEqual final : public std::function<bool(std::string, std::string)>
{
  bool operator()(const char* lhs, const char* rhs) const {
    return _stricmp(lhs, rhs) == 0;
  }

  bool operator()(const std::string& lhs, const std::string& rhs) const {
    return _stricmp(lhs.c_str(), rhs.c_str()) == 0;
  }
};

void identifierToLower(std::string& str);

template<typename T>
using caseless_unordered_set = typename std::unordered_set<T, CaselessStringHasher, CaselessStringEqual>;
template<typename K, typename V>
using caseless_unordered_map = typename std::unordered_map<K, V, CaselessStringHasher, CaselessStringEqual>;
template<typename K, typename V>
using caseless_map = typename std::map<K, V, CaselessStringComparer>;

template<typename T>
using caseless_unordered_identifier_set = typename std::unordered_set<T, CaselessIdentifierHasher, CaselessStringEqual>;
template<typename K, typename V>
using caseless_unordered_identifier_map = typename std::unordered_map<K, V, CaselessIdentifierHasher, CaselessStringEqual>;
template<typename K, typename V>
using caseless_identifier_map = typename std::map<K, V, CaselessStringComparer>;

}
