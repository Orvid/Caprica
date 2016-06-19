#include <common/allocators/ReffyStringPool.h>

#include <assert.h>

namespace caprica { namespace allocators {

size_t ReffyStringPool::lookup(boost::string_ref str) {
  auto h = hash(str);
  auto entry = find(str, h);
  if (entry->generationNum == generationNumber)
    return entry->stringIndex;
  return push_back_with_hash(str, h, entry);
}

boost::string_ref ReffyStringPool::byIndex(size_t v) const {
  assert(v < count);
  auto h = strings[v];
  return boost::string_ref((const char*)(h + 1), h->length);
}

void ReffyStringPool::push_back(boost::string_ref str) {
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

ReffyStringPool::HashEntry* ReffyStringPool::find(boost::string_ref str, size_t hash) {
  HashEntry* entry = &hashtable[hash & mask];
  while (entry->generationNum == generationNumber) {
    if (entry->upperHash == (uint16_t)(hash >> 16) && str == byIndex(entry->stringIndex))
      break;
    if (++entry == &hashtable[MaxCapacity])
      entry = &hashtable[0];
  }
  return entry;
}

size_t ReffyStringPool::push_back_with_hash(boost::string_ref str, size_t hash, HashEntry* entry) {
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

size_t ReffyStringPool::hash(boost::string_ref str) {
  constexpr size_t offsetBasis = 0xcbf29ce484222325ULL;
  constexpr size_t prime = 0x100000001B3ULL;
  const char* cStr = str.data();
  const char* eStr = cStr + str.size();

  size_t val = offsetBasis;
  for (; cStr < eStr; cStr++) {
    val ^= (size_t)*cStr;
    val *= prime;
  }
  return val;
}

}}
