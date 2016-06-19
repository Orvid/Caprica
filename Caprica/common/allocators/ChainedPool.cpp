#include <common/allocators/ChainedPool.h>

#include <common/CapricaReportingContext.h>

namespace caprica { namespace allocators {

ChainedPool::Heap::Heap(size_t heapSize) : allocedHeapSize(heapSize), freeBytes(heapSize) {
  baseAlloc = malloc(heapSize);
  if (!baseAlloc)
    CapricaReportingContext::logicalFatal("Failed to allocate a Heap!");
}

ChainedPool::Heap::~Heap() {
  if (baseAlloc) {
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

char* ChainedPool::allocate(size_t size) {
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

void ChainedPool::reset() {
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

}}
