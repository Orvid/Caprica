#pragma once

#include <boost/container/static_vector.hpp>

namespace caprica { namespace allocators {

template<typename T, size_t heapSize>
struct ChainedDestructedPooledAllocator final
{
  static_assert(heapSize != 0, "A zero sized heap is useless.");

private:
  struct Heap final
  {
    boost::container::static_vector<T, heapSize> heap{ };
    Heap* next{ nullptr };

    // This has to be an explicit constructor
    // to prevent heap being fully zero-initialized.
    Heap() { }
    ~Heap() {
      if (next != nullptr)
        delete next;
    }
  };

  struct HeapIterator final
  {
    size_t index{ 0 };

    HeapIterator& operator ++() {
      if (curHeap == nullptr)
        return *this;
      index++;
      if (heapI + 1 < curHeap->heap.size()) {
        heapI++;
        return *this;
      }
      heapI = 0;
      if (curHeap->heap.size() < heapSize) {
        index = 0;
        curHeap = nullptr;
        return *this;
      }
      curHeap = curHeap->next;
      if (curHeap && !curHeap->heap.size()) {
        index = 0;
        curHeap = nullptr;
      }
      return *this;
    }

    T* operator *() {
      return &curHeap->heap[heapI];
    }
    const T* operator *() const {
      return &curHeap->heap[heapI];
    }
    T* operator ->() {
      return &curHeap->heap[heapI];
    }
    const T* operator ->() const {
      return &curHeap->heap[heapI];
    }

    bool operator !=(const HeapIterator& other) const {
      return curHeap != other.curHeap || heapI != other.heapI;
    }

  private:
    friend ChainedDestructedPooledAllocator;

    Heap* curHeap{ nullptr };
    size_t heapI{ 0 };
  };

  size_t count{ 0 };
  Heap* current{ nullptr };
  Heap* base{ nullptr };

public:
  ChainedDestructedPooledAllocator() = default;
  ChainedDestructedPooledAllocator(const ChainedDestructedPooledAllocator&) = delete;
  ChainedDestructedPooledAllocator(ChainedDestructedPooledAllocator&& o) {
    std::swap(count, o.count);
    std::swap(current, o.current);
    std::swap(base, o.base);
  }
  ChainedDestructedPooledAllocator& operator =(const ChainedDestructedPooledAllocator&) = delete;
  ChainedDestructedPooledAllocator& operator =(ChainedDestructedPooledAllocator&& o) {
    std::swap(count, o.count);
    std::swap(current, o.current);
    std::swap(base, o.base);
    return *this;
  }
  ~ChainedDestructedPooledAllocator() {
    if (base != nullptr)
      delete base;
  }

  template<typename... Args>
  T& emplace_back(Args&&... args) {
    ensure_space_for_emplace();
    count++;
    current->heap.emplace_back(std::forward<Args>(args)...);
    return current->heap.back();
  }

  void reserve(size_t i) {
    if (count >= i)
      return;
    if (base == nullptr)
      current = base = new Heap();
    auto remaining = i - count;
    auto cur = current;
    while (remaining > 0) {
      while (cur->heap.size() >= heapSize) {
        if (cur->next == nullptr) {
          Heap* next = new Heap();
          cur->next = next;
        }
        cur = cur->next;
      }
      assert(cur != nullptr);
      assert(cur->heap.size() < heapSize);
      if (remaining < heapSize - cur->heap.size())
        return;
      auto rem2 = remaining - (heapSize - cur->heap.size());
      assert(rem2 < remaining);
      remaining = rem2;
    }
  }

  HeapIterator begin() const {
    if (!count)
      return HeapIterator();
    HeapIterator iter{ };
    iter.curHeap = base;
    return iter;
  }

  HeapIterator end() const {
    return HeapIterator();
  }

  size_t size() const { return count; }

private:
  void ensure_space_for_emplace() {
    if (base == nullptr) {
      base = new Heap();
      current = base;
    } else if (current->heap.size() >= heapSize) {
      Heap* next = new Heap();
      current->next = next;
      current = next;
    }
  }
};

}}
