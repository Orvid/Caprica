#pragma once

#include <stdlib.h>

#include <atomic>

#include <common/CapricaReportingContext.h>

namespace caprica {

struct HugeConcurrentPooledBufferAllocator final
{
  explicit HugeConcurrentPooledBufferAllocator(size_t hpSize) : heapSize(hpSize), base(hpSize) { }
  ~HugeConcurrentPooledBufferAllocator() = default;

  char* allocate(size_t size);

private:
  struct Heap final {
    size_t allocedHeapSize;
    std::atomic<size_t> freeBytes;
    void* baseAlloc;
    std::atomic<Heap*> next{ nullptr };

    Heap() = delete;
    Heap(const Heap&) = delete;
    Heap(Heap&&) = delete;
    Heap& operator=(const Heap&) = delete;
    Heap& operator=(Heap&&) = delete;
    explicit Heap(size_t heapSize) : allocedHeapSize(heapSize), freeBytes(heapSize) {
      baseAlloc = calloc(1, heapSize);
      if (!baseAlloc)
        CapricaReportingContext::logicalFatal("Failed to allocate a Heap!");
    }
    ~Heap() {
      if (baseAlloc) {
        free(baseAlloc);
        baseAlloc = nullptr;
      }

      if (next.load()) {
        delete next.load();
        next = nullptr;
      }
    }

    bool tryAlloc(size_t size, void** retBuf);
  };

  size_t heapSize;
  Heap base;
  std::atomic<Heap*> current{ &base };

  void* allocHeap(size_t newHeapSize, size_t firstAllocSize);
};

}
