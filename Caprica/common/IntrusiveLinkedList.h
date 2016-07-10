#pragma once

namespace caprica {

template<typename T>
struct IntrusiveLinkedList final
{
  void push_back(T* val) {
    mSize++;
    if (front == nullptr) {
      front = back = val;
    } else {
      back->next = val;
      back = val;
    }
  }

  size_t size() const { return mSize; }

private:
  size_t mSize{ 0 };
  T* front{ nullptr };
  T* back{ nullptr };

  struct ConstIterator final
  {
    ConstIterator& operator ++() {
      if (cur == nullptr)
        return *this;
      cur = cur->next;
      return *this;
    }

    const T*& operator *() {
      return cur;
    }
    const T*& operator *() const {
      return cur;
    }
    const T*& operator ->() {
      return cur;
    }
    const T*& operator ->() const {
      return cur;
    }

    bool operator ==(const ConstIterator& other) const {
      return cur == other.cur;
    }

    bool operator !=(const ConstIterator& other) const {
      return !(*this == other);
    }

  private:
    friend IntrusiveLinkedList;
    const T* cur{ nullptr };

    ConstIterator() = default;
    ConstIterator(const T* front) : cur(front) { }
  };

  struct Iterator final
  {
    Iterator& operator ++() {
      if (cur == nullptr)
        return *this;
      cur = cur->next;
      return *this;
    }

    T*& operator *() {
      return cur;
    }
    const T*& operator *() const {
      return cur;
    }
    T*& operator ->() {
      return cur;
    }
    const T*& operator ->() const {
      return cur;
    }

    bool operator ==(const Iterator& other) const {
      return cur == other.cur;
    }

    bool operator !=(const Iterator& other) const {
      return !(*this == other);
    }

  private:
    friend IntrusiveLinkedList;
    T* cur{ nullptr };

    Iterator() = default;
    Iterator(T* front) : cur(front) { }
  };

public:
  ConstIterator begin() const {
    if (!mSize)
      return ConstIterator();
    return ConstIterator(front);
  }

  ConstIterator end() const {
    return ConstIterator();
  }

  Iterator begin() {
    if (!mSize)
      return Iterator();
    return Iterator(front);
  }

  Iterator end() {
    return Iterator();
  }
};

}
