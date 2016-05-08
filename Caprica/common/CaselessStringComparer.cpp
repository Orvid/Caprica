#include <common/CaselessStringComparer.h>

namespace caprica {

alignas(128) const uint64_t charToLowerMap[] = {
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
  0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
  0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
  0x78, 0x79, 0x7A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
  0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
  0x78, 0x79, 0x7A
};

void identifierToLower(std::string& str) {
  char* data = (char*)str.c_str();
  auto end = str.size();
  for (size_t i = 0; i < end; i++) {
    *data = (char)(charToLowerMap - 0x20)[*data];
  }
}

size_t CaselessStringHasher::doCaselessHash(const char* k, size_t len) {
  // Using FNV-1a hash, the same as the MSVC std lib hash of strings.
  static_assert(sizeof(size_t) == 8, "This is 64-bit only!");
  constexpr size_t offsetBasis = 0xcbf29ce484222325ULL;
  constexpr size_t prime = 0x100000001B3ULL;
  const char* cStr = k;
  const char* eStr = k + len;


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
    val ^= (size_t)(charToLowerMap - 0x20)[*cStr];
    val *= prime;
  }
#endif
  return val;
}

size_t CaselessIdentifierHasher::doIdentifierHash(const char* s, size_t len) {
  const char* cStr = s;

  // We know the string is null-terminated, so we can align to 2.
  size_t lenLeft = ((len + 1) & 0xFFFFFFFFFFFFFFFEULL);
  size_t iterCount = lenLeft >> 2;
  uint32_t val = 0x84222325U;
  size_t i = iterCount;
  while (i)
    val = _mm_crc32_u32(val, ((uint32_t*)cStr)[--i] | 0x20202020);
  if (lenLeft & 2)
    val = _mm_crc32_u16(val, *(uint16_t*)(cStr + (iterCount * 4)) | (uint16_t)0x2020);
  return ((size_t)val << 32) | val;
}

alignas(128) static const __m128i spaces{
  ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' '
};
bool idEq(const char* a, size_t aLen, const char* b, size_t bLen) {
  // This uses the SSE2 instructions movdqa, movdqu, pcmpeqb, por, pmovmskb,
  // and is safe to use on any 64-bit CPU.
  // This returns via goto's because of how MSVC does codegen.
  if (aLen != bLen)
    goto ReturnFalse;
  if (a == b)
    goto ReturnTrue;
  const char* __restrict strA = a;
  int64_t strBOff = b - a;
  // We know the string is null-terminated, so we can align to 2.
  size_t lenLeft = ((aLen + 1) & 0xFFFFFFFFFFFFFFFEULL);
  while (lenLeft >= 16) {
    auto vA = _mm_or_si128(_mm_loadu_si128((__m128i*)strA), spaces);
    auto vB = _mm_or_si128(_mm_loadu_si128((__m128i*)(strA + strBOff)), spaces);
    if (_mm_movemask_epi8(_mm_cmpeq_epi8(vA, vB)) != 0xFFFF)
      goto ReturnFalse;
    strA += 16;
    lenLeft -= 16;
  }
  // We don't need to adjust the lenLeft for each of these.
  if (lenLeft & 8) {
    if ((*(uint64_t*)strA | 0x2020202020202020ULL) != (*(uint64_t*)(strA + strBOff) | 0x2020202020202020ULL))
      goto ReturnFalse;
    strA += 8;
  }
  if (lenLeft & 4) {
    if ((*(uint32_t*)strA | 0x20202020U) != (*(uint32_t*)(strA + strBOff) | 0x20202020U))
      goto ReturnFalse;
    strA += 4;
  }
  if (lenLeft & 2) {
    if ((*(uint16_t*)strA | 0x2020) != (*(uint16_t*)(strA + strBOff) | 0x2020))
      goto ReturnFalse;
  }
ReturnTrue:
  return true;
ReturnFalse:
  return false;
}

bool idEq(const char* a, const char* b) {
  return idEq(a, strlen(a), b, strlen(b));
}
bool idEq(const char* a, const std::string& b) {
  return idEq(a, strlen(a), b.c_str(), b.size());
}
bool idEq(const std::string& a, const char* b) {
  return idEq(a.c_str(), a.size(), b, strlen(b));
}
bool idEq(const std::string& a, const std::string& b) {
  return idEq(a.c_str(), a.size(), b.c_str(), b.size());
}

}
