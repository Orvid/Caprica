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
  size_t operator()(const std::string& k) const {
    // Using FNV-1a hash, the same as the MSVC std lib hash of strings.
    static_assert(sizeof(size_t) == 8, "This is 64-bit only!");
    constexpr size_t offsetBasis = 0xcbf29ce484222325ULL;
    constexpr size_t prime = 0x100000001B3ULL;
    const char* cStr = k.c_str();
    const char* eStr = k.c_str() + k.size();
    

    size_t val = offsetBasis;
#if 0
    for (; cStr < eStr; cStr++) {
      // This is safe only because we are hashing
      // identifiers with a known set of characters.
      val ^= (size_t)(*cStr | 32);
      val *= prime;
    }
#else
    for (; cStr < eStr; cStr++) {
      val ^= (size_t)(charMap - 0x20)[*cStr];
      val *= prime;
    }
#endif
    return val;
  }
private:
  alignas(64) static const uint64_t charMap[];
};

struct CaselessIdentifierHasher final : public std::function<size_t(std::string)>
{
  size_t operator()(const std::string& k) const {
    const char* cStr = k.c_str();

    // We know the string is null-terminated, so we can align to 2.
    size_t lenLeft = k.size() & 0xFFFFFFFFFFFFFFFEULL;
    size_t iterCount = lenLeft >> 2;
    uint32_t val = 0x84222325U;
    size_t i = iterCount;
    while (i)
      val = _mm_crc32_u32(val, ((uint32_t*)cStr)[--i] | 0x20202020);
    if (lenLeft & 2)
      val = _mm_crc32_u16(val, *(uint16_t*)(cStr + (iterCount * 4)) | (uint16_t)0x2020);
    return ((size_t)val << 32) | val;
  }
};

//using CaselessStringHasher = CaselessIdentifierHasher;

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
template<typename K, typename V>
using caseless_map = typename std::map<K, V, CaselessStringComparer>;

template<typename T>
using caseless_unordered_identifier_set = typename std::unordered_set<T, CaselessIdentifierHasher, CaselessStringEqual>;
template<typename K, typename V>
using caseless_unordered_identifier_map = typename std::unordered_map<K, V, CaselessIdentifierHasher, CaselessStringEqual>;
template<typename K, typename V>
using caseless_identifier_map = typename std::map<K, V, CaselessStringComparer>;

}
