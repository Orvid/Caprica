#include <common/allocators/ReffyStringPool.h>

#include <assert.h>

namespace caprica { namespace allocators {

size_t ReffyStringPool::lookup(const identifier_ref& str) {
  auto h = hash(str);
  auto entry = find(str, h);
  if (entry->generationNum == generationNumber)
    return entry->stringIndex;
  return push_back_with_hash(str, h, entry);
}

identifier_ref ReffyStringPool::byIndex(size_t v) const {
  assert(v < count);
  auto h = strings[v];
  return identifier_ref((const char*)(h + 1), h->length);
}

void ReffyStringPool::push_back(const identifier_ref& str) {
  auto h = hash(str);
  push_back_with_hash(str, h, find(str, h));
}

void ReffyStringPool::reset() {
  generationNumber++;
  for (size_t i = 0; i < count; i++)
    strings[i] = nullptr;
  count = 0;
  alloc.reset();
}

ReffyStringPool::HashEntry* ReffyStringPool::find(const identifier_ref& str, size_t hash) {
  HashEntry* entry = &hashtable[hash & mask];
  while (entry->generationNum == generationNumber) {
    if (entry->upperHash == (uint16_t)(hash >> 16) && str == byIndex(entry->stringIndex))
      break;
    if (++entry == &hashtable[MaxCapacity])
      entry = &hashtable[0];
  }
  return entry;
}

size_t ReffyStringPool::push_back_with_hash(const identifier_ref& str, size_t hash, HashEntry* entry) {
  auto hdr = (StringHeader*)alloc.allocate(sizeof(StringHeader) + str.size());
  hdr->length = (uint16_t)str.size();
  hdr->data = (const char*)(hdr + 1);
  memcpy((void*)hdr->data, str.data(), str.size());
  auto ret = count;
  count++;
  strings[ret] = hdr;
  entry->generationNum = generationNumber;
  entry->upperHash = (uint16_t)(hash >> 16);
  entry->stringIndex = (uint16_t)ret;
  return ret;
}

size_t ReffyStringPool::hash(const identifier_ref& str) {
  const char* cStr = str.data();
  size_t lenLeft = str.size();
  size_t iterCount = lenLeft >> 2;
  uint32_t val = 0x84222325U;
  size_t i = iterCount;
  while (i)
    val = _mm_crc32_u32(val, ((uint32_t*)cStr)[--i]);
  if (lenLeft & 2) {
    val = _mm_crc32_u16(val, *(uint16_t*)(cStr + (iterCount * 4)));
    // This is duplicated like this because it ends up cost-free when compared to
    // any other methods.
    if (lenLeft & 1)
      val = _mm_crc32_u8(val, *(uint8_t*)(cStr + (iterCount * 4) + 2));
  } else if (lenLeft & 1) {
    val = _mm_crc32_u8(val, *(uint8_t*)(cStr + (iterCount * 4)));
  }
  return ((size_t)val << 32) | val;
}

}}
