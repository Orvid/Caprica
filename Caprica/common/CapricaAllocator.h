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


template<typename T, size_t heapSize>
struct FlatishDestructedPooledBufferAllocator final
{
private:
  struct Heap final
  {
    boost::container::static_vector<T, heapSize> heap;
    Heap* next{ nullptr };

    Heap() { }
    ~Heap() {
      if (next != nullptr)
        delete next;
    }
  };

  size_t count{ 0 };
  Heap* current{ &base };
  Heap base;

public:
  FlatishDestructedPooledBufferAllocator() { }
  ~FlatishDestructedPooledBufferAllocator() = default;

  template<typename... Args>
  void emplace_back(Args&&... args) {
    if (current->heap.size() >= current->heap.capacity()) {
      Heap* next = new Heap();
      current->next = next;
      current = next;
    }
    count++;
    current->heap.emplace_back(std::forward<Args>(args)...);
  }

  size_t size() const { return count; }

  template<typename F>
  void evacuate(F func) {
    auto cur = &base;
    while (cur != nullptr) {
      for (auto&& r : cur->heap) {
        func(std::move(r));
      }
      cur = cur->next;
    }
  }
};

}
