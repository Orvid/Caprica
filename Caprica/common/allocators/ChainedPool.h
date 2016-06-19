#pragma once

#include <stdlib.h>

namespace caprica { namespace allocators {

struct ChainedPool final
{
  explicit ChainedPool(size_t hpSize) : heapSize(hpSize), base(hpSize) { }
  ~ChainedPool() = default;

  char* allocate(size_t size);
  void reset();

private:
  struct Heap final
  {
    size_t allocedHeapSize;
    size_t freeBytes;
    void* baseAlloc;
    Heap* next{ nullptr };

    Heap() = delete;
    Heap(const Heap&) = delete;
    Heap(Heap&&) = delete;
    Heap& operator=(const Heap&) = delete;
    Heap& operator=(Heap&&) = delete;

    explicit Heap(size_t heapSize);
    ~Heap();

    bool tryAlloc(size_t size, void** retBuf);
  };

  size_t heapSize;
  Heap* current{ &base };
  Heap base;

  void* allocHeap(size_t newHeapSize, size_t firstAllocSize);
};

}}
