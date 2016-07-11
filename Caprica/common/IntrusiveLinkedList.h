#pragma once

namespace caprica {

template<typename T>
struct IntrusiveLinkedList final
{
  T* back() { return mBack; }
  const T* back() const { return mBack; }

  T* front() { return mFront; }
  const T* front() const { return mFront; }

  void push_back(T* val) {
    mSize++;
    if (mFront == nullptr) {
      mFront = mBack = val;
    } else {
      mBack->next = val;
      mBack = val;
    }
  }

  size_t size() const { return mSize; }

private:
  size_t mSize{ 0 };
  T* mFront{ nullptr };
  T* mBack{ nullptr };

  struct ConstIterator final
  {
    size_t index{ 0 };

    ConstIterator& operator ++() {
      if (cur == nullptr)
        return *this;
      index++;
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
    ConstIterator(const T* mFront) : cur(mFront) { }
  };

  struct Iterator final
  {
    size_t index{ 0 };

    Iterator& operator ++() {
      if (cur == nullptr)
        return *this;
      index++;
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
    Iterator(T* mFront) : cur(mFront) { }
  };

public:
  ConstIterator begin() const {
    if (!mSize)
      return ConstIterator();
    return ConstIterator(mFront);
  }

  ConstIterator end() const {
    return ConstIterator();
  }

  Iterator begin() {
    if (!mSize)
      return Iterator();
    return Iterator(mFront);
  }

  Iterator end() {
    return Iterator();
  }
};

}
