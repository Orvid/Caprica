#include <common/CapricaAllocator.h>

namespace caprica {

ConcurrentPooledBufferAllocator::Heap::Heap(size_t heapSize) : allocedHeapSize(heapSize), freeBytes(heapSize) {
  baseAlloc = malloc(heapSize);
  if (!baseAlloc)
    CapricaReportingContext::logicalFatal("Failed to allocate a Heap!");
}

ConcurrentPooledBufferAllocator::Heap::~Heap() {
  if (baseAlloc) {
    free(baseAlloc);
    baseAlloc = nullptr;
  }

  if (next.load()) {
    delete next.load();
    next = nullptr;
  }
}

bool ConcurrentPooledBufferAllocator::Heap::tryAlloc(size_t size, void** retBuf) {
  auto fb = freeBytes.load(std::memory_order_acquire);
  auto newFree = fb - size;
  while (fb > size) {
    if (freeBytes.compare_exchange_weak(fb, newFree)) {
      *retBuf = (void*)((size_t)baseAlloc + allocedHeapSize - fb);
      return true;
    }
    newFree = fb - size;
  }
  return false;
}

char* ConcurrentPooledBufferAllocator::allocate(size_t size) {
  if (size > heapSize) {
    return (char*)allocHeap(size, size);
  }
Again:
  void* ret = nullptr;
  auto curHp = current.load(std::memory_order_acquire);
  if (!curHp->tryAlloc(size, &ret)) {
    auto n = curHp->next.load(std::memory_order_acquire);
    if (n != nullptr) {
      // Whether we are successful or not, current has changed,
      // and we should try our allocation again.
      current.compare_exchange_strong(curHp, n);
      goto Again;
    }
    return (char*)allocHeap(heapSize, size);
  }
  return (char*)ret;
}

void* ConcurrentPooledBufferAllocator::allocHeap(size_t newHeapSize, size_t firstAllocSize) {
  void* ret = nullptr;
  auto hp = new Heap(newHeapSize);
  if (!hp->tryAlloc(firstAllocSize, &ret))
    CapricaReportingContext::logicalFatal("Failed while allocating a Heap!");

  auto curHp = current.load(std::memory_order_acquire);
  auto n = curHp->next.load(std::memory_order_acquire);
  while (n) {
    curHp = n;
    n = curHp->next.load(std::memory_order_acquire);
  }

  while (!curHp->next.compare_exchange_weak(n, hp)) {
    curHp = n;
    n = curHp->next.load(std::memory_order_acquire);
  }

  return ret;
}

void ReffyStringPool::push_back(boost::string_ref str) {
  auto newStr = boost::string_ref(alloc.allocate(str.size()), str.size());
  memcpy((void*)newStr.data(), str.data(), str.size());

  allocedStrings.emplace_back(newStr);
  idxLookup.emplace(newStr, allocedStrings.size() - 1);
}

size_t ReffyStringPool::lookup(boost::string_ref str) {
  auto f = idxLookup.find(str);
  if (f != idxLookup.end())
    return f->second;
  auto newStr = boost::string_ref(alloc.allocate(str.size()), str.size());
  memcpy((void*)newStr.data(), str.data(), str.size());

  allocedStrings.emplace_back(newStr);
  idxLookup.emplace(newStr, allocedStrings.size() - 1);
  return allocedStrings.size() - 1;
}

}
