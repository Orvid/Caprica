#pragma once

#include <cstdint>
#include <cstdlib>

#include <iterator>
#include <string>
#include <string_view>

#include <fmt/format.h>

#include <common/UtilMacros.h>

namespace caprica {

// Yes, the name is identifier_ref, but it is used to hold
// normal string refs as well.
struct identifier_ref final {
  static constexpr size_t npos = size_t(-1);

  constexpr identifier_ref() = default;
  ALWAYS_INLINE
  identifier_ref(const char* str) : identifier_ref(str, strlen(str)) { }
  ALWAYS_INLINE
  identifier_ref(const std::string& str) : identifier_ref(str.data(), str.size()) { }
  ALWAYS_INLINE
  identifier_ref(std::string_view str) : identifier_ref(str.data(), str.size()) { }
  constexpr identifier_ref(const char* str, size_t length) : mData(str), mLength(length) { }
  constexpr identifier_ref(const identifier_ref& rhs) = default;
  constexpr identifier_ref(identifier_ref&& rhs) = default;
  identifier_ref& operator=(const identifier_ref& rhs) = default;
  identifier_ref& operator=(identifier_ref&& rhs) = default;

  constexpr const char* begin() const { return mData; }
  constexpr const char* cbegin() const { return mData; }
  constexpr const char* end() const { return mData + mLength; }
  constexpr const char* cend() const { return mData + mLength; }
  auto rbegin() const { return std::reverse_iterator<const char*>(end()); }
  auto crbegin() const { return std::reverse_iterator<const char*>(end()); }
  auto rend() const { return std::reverse_iterator<const char*>(begin()); }
  auto crend() const { return std::reverse_iterator<const char*>(begin()); }

  constexpr size_t size() const { return mLength; }
  constexpr bool empty() const { return mLength == 0; }

  constexpr const char& operator[](size_t pos) const { return mData[pos]; }

  const char& at(size_t pos) const;

  constexpr const char& front() const { return mData[0]; }
  constexpr const char& back() const { return mData[mLength - 1]; }
  constexpr const char* data() const { return mData; }

  void clear();
  identifier_ref substr(size_t pos, size_t n = npos) const;
  bool identifierEquals(const identifier_ref& s) const;
  uint32_t identifierHash() const;
  bool equals(const identifier_ref& s) const;
  bool starts_with(char c) const;
  bool starts_with(const identifier_ref& s) const;
  bool ends_with(char c) const;
  bool ends_with(const identifier_ref& s) const;
  size_t find(const identifier_ref& s) const;
  size_t find(char c) const;
  size_t rfind(const identifier_ref& s) const;
  size_t rfind(char c) const;
  size_t find_first_of(char c) const;
  size_t find_last_of(char c) const;
  size_t find_first_of(const identifier_ref& s) const;
  size_t find_last_of(const identifier_ref& s) const;
  size_t find_first_not_of(const identifier_ref& s) const;
  size_t find_first_not_of(char c) const;
  size_t find_last_not_of(const identifier_ref& s) const;
  size_t find_last_not_of(char c) const;

  std::string to_string() const;
  std::string_view to_string_view() const;

private:
  const char* mData { nullptr };
  size_t mLength { 0 };
  mutable uint32_t mCaselessHash { 0 };

  size_t reverse_distance(std::reverse_iterator<const char*> first, std::reverse_iterator<const char*> last) const;
};

bool operator==(const identifier_ref& x, const identifier_ref& y);
bool operator==(const identifier_ref& x, const std::string& y);
bool operator==(const std::string& x, const identifier_ref& y);
bool operator==(const identifier_ref& x, const char* y);
bool operator==(const char* x, const identifier_ref& y);
bool operator!=(const identifier_ref& x, const identifier_ref& y);
bool operator!=(const identifier_ref& x, const std::string& y);
bool operator!=(const std::string& x, const identifier_ref& y);
bool operator!=(const identifier_ref& x, const char* y);
bool operator!=(const char* x, const identifier_ref& y);

}

namespace fmt {
template <>
struct formatter<caprica::identifier_ref> {
  constexpr auto parse(format_parse_context& ctx) {
    if (ctx.begin() != ctx.end())
      throw format_error("invalid format");
    return ctx.end();
  }

  template <class FormatContext>
  auto format(const caprica::identifier_ref& str, FormatContext& ctx) const {
    return fmt::format_to(ctx.out(), "{}", str.to_string_view());
  }
};
}
