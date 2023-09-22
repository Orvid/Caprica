#include <common/identifier_ref.h>

#include <assert.h>

#include <algorithm>
#include <exception>
#include <limits>
#include <stdexcept>

#include <common/CaselessStringComparer.h>

namespace caprica {

template <typename Iterator>
static Iterator find_not_of(Iterator first, Iterator last, const char* sData, size_t sLength) {
  assert(sLength <= std::numeric_limits<int>::max());
  if (!sLength)
    return first;
  for (; first != last; ++first)
    if (std::memchr(sData, *first, (int)sLength) == 0)
      return first;
  return last;
}

const char& identifier_ref::at(size_t pos) const {
  if (pos >= mLength)
    throw std::out_of_range("identifier_ref::at");
  return mData[pos];
}

void identifier_ref::clear() {
  mLength = 0;
}

identifier_ref identifier_ref::substr(size_t pos, size_t n) const {
  if (pos > size())
    throw std::out_of_range("identifier_ref::substr");
  if (n == npos || pos + n > size())
    n = size() - pos;
  return identifier_ref(data() + pos, n);
}

bool identifier_ref::identifierEquals(const identifier_ref& s) const {
  if (mLength != s.mLength)
    return false;
  if (identifierHash() != s.identifierHash())
    return false;
  return CaselessIdentifierEqual::equal<false>(mData, s.mData, mLength);
}

uint32_t identifier_ref::identifierHash() const {
  if (mCaselessHash != 0)
    return mCaselessHash;
  mCaselessHash = CaselessIdentifierHasher::hash<false>(mData, mLength);
  if (!mCaselessHash)
    mCaselessHash = 1;
  return mCaselessHash;
}
bool identifier_ref::equals(const identifier_ref& s) const {
  if (mLength != s.mLength)
    return false;
  return memcmp(mData, s.mData, std::min(mLength, s.mLength)) == 0;
}

bool identifier_ref::starts_with(char c) const {
  return !empty() && c == front();
}

bool identifier_ref::starts_with(const identifier_ref& x) const {
  return mLength >= x.mLength && strncmp(mData, x.mData, x.mLength) == 0;
}

bool identifier_ref::ends_with(char c) const {
  return !empty() && c == back();
}

bool identifier_ref::ends_with(const identifier_ref& x) const {
  return mLength >= x.mLength && strncmp(mData + mLength - x.mLength, x.mData, x.mLength) == 0;
}

size_t identifier_ref::find(const identifier_ref& s) const {
  auto iter = std::search(this->cbegin(), this->cend(), s.cbegin(), s.cend());
  return iter == this->cend() ? npos : std::distance(this->cbegin(), iter);
}

size_t identifier_ref::find(char c) const {
  auto iter = std::find_if(this->cbegin(), this->cend(), [c](char other) { return c == other; });
  return iter == this->cend() ? npos : std::distance(this->cbegin(), iter);
}

size_t identifier_ref::rfind(const identifier_ref& s) const {
  auto iter = std::search(this->crbegin(), this->crend(), s.crbegin(), s.crend());
  return iter == this->crend() ? npos : reverse_distance(this->crbegin(), iter);
}

size_t identifier_ref::rfind(char c) const {
  auto iter = std::find_if(this->crbegin(), this->crend(), [c](char other) { return c == other; });
  return iter == this->crend() ? npos : reverse_distance(this->crbegin(), iter);
}

size_t identifier_ref::find_first_of(char c) const {
  return find(c);
}

size_t identifier_ref::find_last_of(char c) const {
  return rfind(c);
}

size_t identifier_ref::find_first_of(const identifier_ref& s) const {
  auto iter = std::find_first_of(this->cbegin(), this->cend(), s.cbegin(), s.cend());
  return iter == this->cend() ? npos : std::distance(this->cbegin(), iter);
}

size_t identifier_ref::find_last_of(const identifier_ref& s) const {
  auto iter = std::find_first_of(this->crbegin(), this->crend(), s.cbegin(), s.cend());
  return iter == this->crend() ? npos : reverse_distance(this->crbegin(), iter);
}

size_t identifier_ref::find_first_not_of(const identifier_ref& s) const {
  auto iter = find_not_of(this->cbegin(), this->cend(), s.mData, s.mLength);
  return iter == this->cend() ? npos : std::distance(this->cbegin(), iter);
}

size_t identifier_ref::find_first_not_of(char c) const {
  for (auto iter = this->cbegin(); iter != this->cend(); ++iter)
    if (c != *iter)
      return std::distance(this->cbegin(), iter);
  return npos;
}

size_t identifier_ref::find_last_not_of(const identifier_ref& s) const {
  auto iter = find_not_of(this->crbegin(), this->crend(), s.mData, s.mLength);
  return iter == this->crend() ? npos : reverse_distance(this->crbegin(), iter);
}

size_t identifier_ref::find_last_not_of(char c) const {
  for (auto iter = this->crbegin(); iter != this->crend(); ++iter)
    if (c != *iter)
      return reverse_distance(this->crbegin(), iter);
  return npos;
}

std::string identifier_ref::to_string() const {
  return std::string(mData, mLength);
}

std::string_view identifier_ref::to_string_view() const {
  return std::string_view(mData, mLength);
}

size_t identifier_ref::reverse_distance(std::reverse_iterator<const char*> first,
                                        std::reverse_iterator<const char*> last) const {
  return mLength - 1 - std::distance(first, last);
}

bool operator==(const identifier_ref& x, const identifier_ref& y) {
  return x.equals(y);
}

bool operator==(const identifier_ref& x, const std::string& y) {
  return x == identifier_ref(y);
}

bool operator==(const std::string& x, const identifier_ref& y) {
  return identifier_ref(x) == y;
}

bool operator==(const identifier_ref& x, const char* y) {
  return x == identifier_ref(y);
}

bool operator==(const char* x, const identifier_ref& y) {
  return identifier_ref(x) == y;
}

bool operator!=(const identifier_ref& x, const identifier_ref& y) {
  return !x.equals(y);
}

bool operator!=(const identifier_ref& x, const std::string& y) {
  return x != identifier_ref(y);
}

bool operator!=(const std::string& x, const identifier_ref& y) {
  return identifier_ref(x) != y;
}

bool operator!=(const identifier_ref& x, const char* y) {
  return x != identifier_ref(y);
}

bool operator!=(const char* x, const identifier_ref& y) {
  return identifier_ref(x) != y;
}

}
