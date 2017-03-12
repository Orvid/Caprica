#include <common/allocators/ChainedPool.h>

#include <common/CapricaReportingContext.h>
#include <common/CapricaStats.h>

namespace caprica { namespace allocators {

ChainedPool::~ChainedPool() {
  auto curNode = rootDestructorChain;
  while (curNode != nullptr) {
    curNode->destructor((void*)((size_t)curNode + sizeof(DestructionNode)));
    curNode = curNode->next;
  }
}

ChainedPool::Heap::Heap(size_t heapSize) : allocedHeapSize(heapSize), freeBytes(heapSize) {
  CapricaStats::allocatedHeapCount++;
  baseAlloc = malloc(heapSize);
  if (!baseAlloc)
    CapricaReportingContext::logicalFatal("Failed to allocate a Heap!");
}

ChainedPool::Heap::~Heap() {
  if (baseAlloc) {
    CapricaStats::freedHeapCount++;
    free(baseAlloc);
    baseAlloc = nullptr;
  }

  if (next) {
    delete next;
    next = nullptr;
  }
}

bool ChainedPool::Heap::tryAlloc(size_t size, void** retBuf) {
  if (freeBytes > size) {
    *retBuf = (void*)((size_t)baseAlloc + allocedHeapSize - freeBytes);
    freeBytes -= size;
    return true;
  }
  return false;
}

const char* ChainedPool::HeapIterator::data() const {
  return (const char*)curHeap->baseAlloc;
}

size_t ChainedPool::HeapIterator::size() const {
  return curHeap->allocedHeapSize - curHeap->freeBytes;
}

bool ChainedPool::HeapIterator::operator !=(const HeapIterator& other) const {
  return curHeap != other.curHeap;
}

ChainedPool::HeapIterator& ChainedPool::HeapIterator::operator ++() {
  if (curHeap)
    curHeap = curHeap->next;
  return *this;
}

char* ChainedPool::allocate(size_t size) {
  totalSize += size;
  if (size > heapSize) {
    return (char*)allocHeap(size, size);
  }
Again:
  void* ret = nullptr;
  if (!current->tryAlloc(size, &ret)) {
    if (current->next != nullptr) {
      current = current->next;
      goto Again;
    }
    return (char*)allocHeap(heapSize, size);
  }
  return (char*)ret;
}

std::string_view ChainedPool::allocateString(std::string&& str) {
  return allocateString((char*)str.data(), str.size());
}

std::string_view ChainedPool::allocateString(std::string_view str) {
  return allocateString((char*)str.data(), str.size());
}

std::string_view ChainedPool::allocateString(const char* str, size_t len) {
  auto buf = allocate(len);
  memcpy(buf, str, len);
  return std::string_view(buf, len);
}

identifier_ref ChainedPool::allocateIdentifier(std::string&& str) {
  return allocateIdentifier((char*)str.data(), str.size());
}

identifier_ref ChainedPool::allocateIdentifier(const identifier_ref& str) {
  return allocateIdentifier((char*)str.data(), str.size());
}

identifier_ref ChainedPool::allocateIdentifier(const char* str, size_t len) {
  auto buf = allocate(len);
  memcpy(buf, str, len);
  return identifier_ref(buf, len);
}

void ChainedPool::reset() {
  auto curNode = rootDestructorChain;
  if (curNode != nullptr) {
    while (curNode != nullptr) {
      curNode->destructor((void*)((size_t)curNode + sizeof(DestructionNode)));
      curNode = curNode->next;
    }
    rootDestructorChain = nullptr;
    currentDestructorNode = nullptr;
  }

  current = &base;
  auto c = current;
  while (c && c->freeBytes != c->allocedHeapSize) {
    c->freeBytes = c->allocedHeapSize;
    c = c->next;
  }
}

void* ChainedPool::allocHeap(size_t newHeapSize, size_t firstAllocSize) {
  void* ret = nullptr;
  auto hp = new Heap(newHeapSize);
  if (!hp->tryAlloc(firstAllocSize, &ret))
    CapricaReportingContext::logicalFatal("Failed while allocating a Heap!");

  auto curHp = current;
  while (curHp->next)
    curHp = curHp->next;
  curHp->next = hp;
  return ret;
}

ChainedPool::HeapIterator ChainedPool::begin() const {
  HeapIterator iter{ };
  iter.curHeap = current;
  return iter;
}

ChainedPool::HeapIterator ChainedPool::end() const {
  return HeapIterator();
}

}}
