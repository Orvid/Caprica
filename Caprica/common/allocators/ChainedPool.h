#pragma once

#include <stdlib.h>

#include <memory>
#include <string>
#include <type_traits>

#include <boost/utility/string_ref.hpp>

namespace caprica { namespace allocators {

struct ChainedPool final
{
  explicit ChainedPool(size_t hpSize) : heapSize(hpSize), base(hpSize) { }
  ~ChainedPool();

  char* allocate(size_t size);
  template<typename T, typename... Args>
  T* make(Args&&... args) {
    if (std::is_trivially_destructible<T>::value) {
      auto t = allocate(sizeof(T));
      __assume(t != nullptr);
      return new (t) T(std::forward<Args>(args)...);
    }
    auto buf = allocate(sizeof(DestructionNode) + sizeof(T));
    __assume(buf != nullptr);
    auto node = (DestructionNode*)buf;
    node->destructor = [](void* val) {
      ((T*)val)->T::~T();
    };
    node->next = nullptr;
    if (!rootDestructorChain) {
      rootDestructorChain = node;
      currentDestructorNode = node;
    } else {
      currentDestructorNode->next = node;
      currentDestructorNode = node;
    }
    return new (buf + sizeof(DestructionNode)) T(std::forward<Args>(args)...);
  }
  boost::string_ref allocateString(std::string&& str);
  boost::string_ref allocateString(const char* str, size_t len);

  void reset();
  size_t totalAllocatedBytes() const { return totalSize; }

private:
  friend struct TypedHeapIteratorWrapper;
  friend struct TypedHeapIterator;

  struct DestructionNode final
  {
    void(*destructor)(void*){ nullptr };
    DestructionNode* next{ nullptr };
  };

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

    friend struct HeapIterator;
  };

  struct HeapIterator final
  {
    const char* data() const;
    size_t size() const;
    bool operator !=(const HeapIterator& other) const;
    HeapIterator& operator ++();
  private:
    friend ChainedPool;
    Heap* curHeap{ nullptr };
  };

  template<typename T>
  struct TypedHeapIterator final
  {
    size_t index{ 0 };

    TypedHeapIterator<T>& operator ++() {
      if (curHeap == nullptr)
        return *this;
      index++;
      heapI++;
      if ((char*)heapI < (char*)curHeap->baseAlloc + (curHeap->allocedHeapSize - curHeap->freeBytes))
        return *this;
      heapI = nullptr;
      curHeap = curHeap->next;
      if (curHeap) {
        if (curHeap->freeBytes != curHeap->allocedHeapSize) {
          heapI = (T*)curHeap->baseAlloc;
        } else {
          index = 0;
          curHeap = nullptr;
        }
      }
      return *this;
    }

    T& operator *() {
      return *heapI;
    }
    const T& operator *() const {
      return *heapI;
    }
    T& operator ->() {
      return *heapI;
    }
    const T& operator ->() const {
      return *heapI;
    }

    bool operator ==(const TypedHeapIterator<T>& other) const {
      return curHeap == other.curHeap && heapI == other.heapI;
    }

    bool operator !=(const TypedHeapIterator<T>& other) const {
      return !(*this == other);
    }

  private:
    friend TypedHeapIteratorWrapper;

    Heap* curHeap{ nullptr };
    T* heapI{ nullptr };

    TypedHeapIterator() = default;
    TypedHeapIterator(Heap* cur) : curHeap(cur) {
      if (curHeap->freeBytes != curHeap->allocedHeapSize)
        heapI = (T*)curHeap->baseAlloc;
    }
  };

  template<typename T>
  struct TypedHeapIteratorWrapper final
  {
    TypedHeapIterator<T> begin() const {
      if (!parent->totalSize)
        return TypedHeapIterator<T>();
      return TypedHeapIterator<T>(&parent->base);
    }

    TypedHeapIterator<T> end() const {
      return TypedHeapIterator<T>();
    }

  private:
    friend ChainedPool;
    ChainedPool* parent;
    TypedHeapIteratorWrapper(ChainedPool* p) : parent(p) { }
  };

  size_t heapSize;
  size_t totalSize{ 0 };
  Heap* current{ &base };
  Heap base;
  DestructionNode* rootDestructorChain{ nullptr };
  DestructionNode* currentDestructorNode{ nullptr };

  void* allocHeap(size_t newHeapSize, size_t firstAllocSize);

public:
  HeapIterator begin() const;
  HeapIterator end() const;

  template<typename T>
  TypedHeapIteratorWrapper<T> typedIterator() {
    return TypedHeapIteratorWrapper<T>(this);
  }
};

}}
