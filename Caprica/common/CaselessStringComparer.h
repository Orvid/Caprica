#pragma once

#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
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
  size_t operator()(const std::string& k) const {
    // Using FNV-1a hash, the same as the MSVC std lib hash of strings.
    static_assert(sizeof(size_t) == 8, "This is 64-bit only!");
    constexpr size_t offsetBasis = 0xcbf29ce484222325ULL;
    constexpr size_t prime = 0x100000001B3ULL;

    size_t val = offsetBasis;
    for (size_t i = 0; i < k.size(); i++) {
      val ^= (size_t)charMap[k.c_str()[i] - 0x20];
      val *= prime;
    }
    return val;
  }

private:
  static const uint8_t charMap[];
};

struct CaselessStringEqual final : public std::function<bool(std::string, std::string)>
{
  bool operator()(const std::string &lhs, const std::string &rhs) const {
    return _stricmp(lhs.c_str(), rhs.c_str()) == 0;
  }
};

template<typename T>
using caseless_unordered_set = typename std::unordered_set<T, CaselessStringHasher, CaselessStringEqual>;
template<typename K, typename V>
using caseless_unordered_map = typename std::unordered_map<K, V, CaselessStringHasher, CaselessStringEqual>;

}
