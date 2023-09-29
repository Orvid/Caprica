#pragma once

#include <cctype>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <common/identifier_ref.h>
#include <common/UtilMacros.h>

namespace caprica {

void identifierToLower(char* data, size_t size);
void identifierToLower(std::string& str);

bool caselessEq(std::string_view a, std::string_view b);
bool pathEq(std::string_view a, std::string_view b);
bool pathEq(std::string_view a, const identifier_ref& b);
bool pathEq(const identifier_ref& a, const identifier_ref& b);
bool pathEq(std::string_view a, const char* b);

bool idEq(const char* a, const char* b);
bool idEq(const char* a, const std::string& b);
bool idEq(const std::string& a, const char* b);
NEVER_INLINE
bool idEq(const std::string& a, const std::string& b);
NEVER_INLINE
bool idEq(std::string_view a, std::string_view b);
ALWAYS_INLINE
bool idEq(const identifier_ref& a, const identifier_ref& b) {
  return a.identifierEquals(b);
}

struct CaselessStringHasher final {
  size_t operator()(const char* k) const = delete;
  size_t operator()(const std::string& k) const { return doCaselessHash(k.c_str(), k.size()); }

private:
  friend struct CaselessPathHasher;
  NEVER_INLINE
  static size_t doCaselessHash(const char* k, size_t len);
};

struct CaselessStringEqual final {
  using is_transparent = int;

  bool operator()(const char* lhs, const char* rhs) const = delete;
  bool operator()(const std::string& lhs, const std::string& rhs) const { return caselessEq(lhs, rhs); }
};

struct CaselessPathHasher final {
  size_t operator()(const std::string& k) const { return doPathHash(k.c_str(), k.size()); }

private:
  NEVER_INLINE
  static size_t doPathHash(const char* k, size_t len);
};

struct CaselessPathEqual final {
  bool operator()(const std::string& lhs, const std::string& rhs) const {
    return pathEq(std::string_view(lhs), std::string_view(rhs));
  }
};

struct CaselessIdentifierHasher final {
  using is_transparent = int;

  template <bool isNullTerminated>
  static uint32_t hash(const char* s, size_t len);

  size_t operator()(const char* k) const = delete;
  NEVER_INLINE
  size_t operator()(const std::string& k) const;
  NEVER_INLINE
  size_t operator()(std::string_view k) const;

  size_t operator()(const identifier_ref& k) const {
    uint32_t r = k.identifierHash();
    return ((size_t)r << 32) | r;
  }
};
extern template uint32_t CaselessIdentifierHasher::hash<true>(const char*, size_t);
extern template uint32_t CaselessIdentifierHasher::hash<false>(const char*, size_t);

struct CaselessIdentifierEqual final {
  using is_transparent = int;

  template <bool isNullTerminated>
  static bool equal(const char* a, const char* b, size_t len);

  bool operator()(const char* lhs, const char* rhs) const = delete;
  bool operator()(const std::string& lhs, const std::string& rhs) const { return idEq(lhs, rhs); }
  bool operator()(std::string_view lhs, std::string_view rhs) const { return idEq(lhs, rhs); }
  bool operator()(const identifier_ref& lhs, const identifier_ref& rhs) const { return idEq(lhs, rhs); }
};
extern template bool CaselessIdentifierEqual::equal<true>(const char*, const char*, size_t);
extern template bool CaselessIdentifierEqual::equal<false>(const char*, const char*, size_t);

// These aren't as restricted as identifiers, but must be in the base-ascii
// range, and must not contain control characters.
using caseless_unordered_set = std::unordered_set<std::string, CaselessStringHasher, CaselessStringEqual>;

// The path collections are for UTF-8 encoded strings, and when comparing two,
// they are likely to have a long prefix in common.
using caseless_unordered_path_set = std::unordered_set<std::string, CaselessPathHasher, CaselessPathEqual>;
template <typename V>
using caseless_unordered_path_map = typename std::unordered_map<std::string, V, CaselessPathHasher, CaselessPathEqual>;

using caseless_unordered_identifier_set =
    std::unordered_set<std::string, CaselessIdentifierHasher, CaselessIdentifierEqual>;
using caseless_unordered_identifier_ref_set =
    std::unordered_set<identifier_ref, CaselessIdentifierHasher, CaselessIdentifierEqual>;
template <typename V>
using caseless_unordered_identifier_map =
    typename std::unordered_map<std::string, V, CaselessIdentifierHasher, CaselessIdentifierEqual>;
template <typename V>
using caseless_unordered_identifier_ref_map =
    typename std::unordered_map<identifier_ref, V, CaselessIdentifierHasher, CaselessIdentifierEqual>;

}
