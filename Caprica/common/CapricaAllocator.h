#pragma once

#include <stdlib.h>

#include <atomic>

#include <boost/container/static_vector.hpp>
#include <boost/utility/string_ref.hpp>

#include <common/CapricaReportingContext.h>

namespace caprica {

struct ConcurrentPooledBufferAllocator final
{
  explicit ConcurrentPooledBufferAllocator(size_t hpSize) : heapSize(hpSize), base(hpSize) { }
  ~ConcurrentPooledBufferAllocator() = default;

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

    explicit Heap(size_t heapSize);
    ~Heap();

    bool tryAlloc(size_t size, void** retBuf);
  };

  size_t heapSize;
  std::atomic<Heap*> current{ &base };
  Heap base;

  void* allocHeap(size_t newHeapSize, size_t firstAllocSize);
};

}
