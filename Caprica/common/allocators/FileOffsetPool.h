#pragma once

#include <assert.h>
#include <stdlib.h>

#include <memory>

namespace caprica { namespace allocators {

struct FileOffsetPool final
{
  explicit FileOffsetPool() = default;
  ~FileOffsetPool() {
    if (flattenedTree) {
      free(flattenedTree);
      flattenedTree = nullptr;
      flattenedTreeSize = 0;
    }
  }

  void flatten() {
    if (mSize != flattenedTreeSize) {
        flattenedTree = (size_t*)realloc(flattenedTree, sizeof(size_t) * mSize);

      auto curFlatTree = &flattenedTree[flattenedTreeSize];
      // We also reset the tree as we go.
      current = &base;
      auto c = current;
      while (c && c->nextDataIndex != 0) {
        memcpy(curFlatTree, c->data, sizeof(size_t) * c->nextDataIndex);
        curFlatTree += c->nextDataIndex;
        flattenedTreeSize += c->nextDataIndex;
        c->nextDataIndex = 0;
        c = c->next;
      }
      assert(flattenedTreeSize == mSize);
    }
  }

  void push_back(size_t fileOffset) {
    mSize++;
    if (current->nextDataIndex >= HeapSize) {
      auto newHeap = (Heap*)malloc(sizeof(Heap));
      newHeap->nextDataIndex = 0;
      newHeap->next = nullptr;
      current->next = newHeap;
      current = newHeap;
    }
    current->data[current->nextDataIndex++] = fileOffset;
  }

  size_t size() const { return mSize; }

private:
  static constexpr size_t HeapSize = 510; // Chosen so that sizeof(Heap) == 4096

  struct Heap final
  {
    size_t nextDataIndex{ 0 };
    Heap* next{ nullptr };
    size_t data[HeapSize];

    explicit Heap() { }
    ~Heap() {
      if (next) {
        next->Heap::~Heap();
        free(next);
        next = nullptr;
      }
    }
  };

  size_t mSize{ 0 };
  Heap* current{ &base };
  size_t* flattenedTree{ nullptr };
  size_t flattenedTreeSize{ 0 };
  Heap base;

public:
  struct Iterator final
  {
    using iterator_category = std::random_access_iterator_tag;
    using value_type = size_t;
    using difference_type = int64_t;
    using pointer = size_t*;
    using reference = size_t&;

    size_t* data;
    size_t curIndex{ 0 };
    size_t maxIndex;

    Iterator& operator ++() {
      if (curIndex == maxIndex)
        return *this;
      curIndex++;
      return *this;
    }

    Iterator& operator +=(int64_t off) {
      if (curIndex + off < 0) {
        curIndex = 0;
        return *this;
      }
      if ((size_t)(curIndex + off) > maxIndex) {
        curIndex = maxIndex;
        return *this;
      }
      curIndex += off;
      return *this;
    }

    Iterator operator +(int64_t off) const {
      Iterator ret = *this;
      ret += off;
      return ret;
    }

    Iterator& operator --() {
      if (curIndex == 0)
        return *this;
      curIndex--;
      return *this;
    }

    Iterator& operator -=(int64_t off) {
      if (curIndex - off < 0) {
        curIndex = 0;
        return *this;
      }
      if ((size_t)(curIndex - off) > maxIndex) {
        curIndex = maxIndex;
        return *this;
      }
      curIndex -= off;
      return *this;
    }

    Iterator operator -(int64_t off) const {
      Iterator ret = *this;
      ret -= off;
      return ret;
    }

    int64_t operator -(const Iterator& b) const {
      return (int64_t)curIndex - (int64_t)b.curIndex;
    }

    size_t& operator [](size_t idx) {
      return data[curIndex];
    }
    const size_t& operator [](size_t idx) const {
      return data[curIndex];
    }
    size_t& operator *() {
      return data[curIndex];
    }
    const size_t& operator *() const {
      return data[curIndex];
    }
    size_t& operator ->() {
      return data[curIndex];
    }
    const size_t& operator ->() const {
      return data[curIndex];
    }

    bool operator ==(const Iterator& other) const {
      return curIndex == other.curIndex;
    }
    bool operator !=(const Iterator& other) const { return !(*this == other); }
    bool operator <(const Iterator& other) const { return curIndex < other.curIndex; }
    bool operator <=(const Iterator& other) const { return curIndex <= other.curIndex; }
    bool operator >(const Iterator& other) const { return curIndex > other.curIndex; }
    bool operator >=(const Iterator& other) const { return curIndex >= other.curIndex; }

    explicit Iterator() = default;
  private:
    friend FileOffsetPool;
    Iterator(size_t* dat, size_t curIdx, size_t maxIdx) : data(dat), curIndex(curIdx), maxIndex(maxIdx) { }
  };

  Iterator begin() {
    flatten();
    return Iterator(flattenedTree, 0, flattenedTreeSize);
  }

  Iterator end() {
    flatten();
    return Iterator(flattenedTree, flattenedTreeSize, flattenedTreeSize);
  }

  size_t at(size_t off) {
    flatten();
    return flattenedTree[off];
  }
};

inline FileOffsetPool::Iterator operator +(int64_t off, const FileOffsetPool::Iterator& a) {
  FileOffsetPool::Iterator ret = a;
  ret += off;
  return ret;
}

inline FileOffsetPool::Iterator operator -(int64_t off, const FileOffsetPool::Iterator& a) {
  FileOffsetPool::Iterator ret = a;
  ret -= off;
  return ret;
}

}}
