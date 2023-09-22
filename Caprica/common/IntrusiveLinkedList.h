#pragma once
#include <stdexcept>

namespace caprica {

template <typename T>
struct IntrusiveLinkedList final {
  IntrusiveLinkedList() = default;
  IntrusiveLinkedList(const IntrusiveLinkedList&) = delete;
  IntrusiveLinkedList(IntrusiveLinkedList&&) = default;
  IntrusiveLinkedList& operator=(const IntrusiveLinkedList&) = delete;
  IntrusiveLinkedList& operator=(IntrusiveLinkedList&&) = default;
  ~IntrusiveLinkedList() = default;

  T* back() { return mBack; }
  const T* back() const { return mBack; }

  T* front() { return mFront; }
  const T* front() const { return mFront; }

  void push_back(T* val) {
    val->next = nullptr;
    mSize++;
    if (mFront == nullptr) {
      mFront = mBack = val;
    } else {
      mBack->next = val;
      mBack = val;
    }
  }

  void push_front(T* val) {
    mSize++;
    if (mFront == nullptr) {
      mFront = mBack = val;
    } else {
      val->next = mFront;
      mFront = val;
    }
  }

  T* pop_front() {
    mSize--;
    auto ret = mFront;
    mFront = ret->next;
    if (mFront == nullptr)
      mBack = nullptr;
    return ret;
  }

  size_t size() const { return mSize; }

private:
  size_t mSize { 0 };
  T* mFront { nullptr };
  T* mBack { nullptr };

  struct ConstIterator final {
    size_t index { 0 };

    ConstIterator& operator++() {
      if (cur == nullptr)
        return *this;
      index++;
      cur = cur->next;
      return *this;
    }

    const T*& operator*() { return cur; }
    const T*& operator*() const { return cur; }
    const T*& operator->() { return cur; }
    const T*& operator->() const { return cur; }

    bool operator==(const ConstIterator& other) const { return cur == other.cur; }

    bool operator!=(const ConstIterator& other) const { return !(*this == other); }

  private:
    friend IntrusiveLinkedList;
    const T* cur { nullptr };

    ConstIterator() = default;
    ConstIterator(const T* mFront) : cur(mFront) { }
  };

  struct Iterator final {
    size_t index { 0 };

    Iterator& operator++() {
      if (cur == nullptr)
        return *this;
      index++;
      cur = cur->next;
      return *this;
    }

    T*& operator*() { return cur; }
    const T*& operator*() const { return cur; }
    T*& operator->() { return cur; }
    const T*& operator->() const { return cur; }

    bool operator==(const Iterator& other) const { return cur == other.cur; }

    bool operator!=(const Iterator& other) const { return !(*this == other); }

  private:
    friend IntrusiveLinkedList;
    T* cur { nullptr };

    Iterator() = default;
    Iterator(T* mFront) : cur(mFront) { }
  };

  template <typename T2>
  friend struct ConstLockstepIteratorWrapper;
  template <typename T2>
  friend struct LockstepIteratorWrapper;

public:
  template <typename T2>
  struct LockstepIterator final {
    size_t index { 0 };

    LockstepIterator& operator++() {
      if (cur.self == nullptr)
        return *this;
      index++;
      cur.prevSelf = cur.self;
      cur.prevOther = cur.other;
      cur.self = cur.self->next;
      cur.other = cur.other->next;
      return *this;
    }

    auto& operator*() { return cur; }
    const auto& operator*() const { return cur; }
    auto& operator->() { return cur; }
    const auto& operator->() const { return cur; }

    bool operator==(const LockstepIterator& other) const {
      return cur.self == other.cur.self && cur.other == other.cur.other;
    }

    bool operator!=(const LockstepIterator& other) const { return !(*this == other); }

    LockstepIterator() = default;
    LockstepIterator(T* selfFront, T2* otherFront) {
      cur.self = selfFront;
      cur.other = otherFront;
    }

  private:
    friend LockstepIteratorWrapper<T2>;
    friend ConstLockstepIteratorWrapper<T2>;
    struct {
      T* self { nullptr };
      T2* other { nullptr };
      T* prevSelf { nullptr };
      T2* prevOther { nullptr };
    } cur {};
  };

private:
  template <typename T2>
  struct LockstepIteratorWrapper final {
    LockstepIterator<T2> begin() {
      if (!self.size())
        return LockstepIterator<T2>();
      return LockstepIterator<T2>(self.mFront, other.mFront);
    }

    LockstepIterator<T2> end() { return LockstepIterator<T2>(); }

  private:
    friend IntrusiveLinkedList;
    IntrusiveLinkedList& self;
    IntrusiveLinkedList<T2>& other;

    LockstepIteratorWrapper(IntrusiveLinkedList& pSelf, IntrusiveLinkedList<T2>& pOther)
        : self(pSelf), other(pOther) { }
  };

  template <typename T2>
  struct ConstLockstepIteratorWrapper final {
    LockstepIterator<const T2> begin() {
      if (!self.size())
        return LockstepIterator<const T2>();
      return LockstepIterator<const T2>(self.front(), other.front());
    }

    LockstepIterator<const T2> end() { return LockstepIterator<const T2>(); }

  private:
    friend IntrusiveLinkedList;
    IntrusiveLinkedList& self;
    const IntrusiveLinkedList<T2>& other;

    ConstLockstepIteratorWrapper(IntrusiveLinkedList& pSelf, const IntrusiveLinkedList<T2>& pOther)
        : self(pSelf), other(pOther) { }
  };

  struct InsertableIterator final {
    InsertableIterator& operator++() {
      if (cur == nullptr)
        return *this;
      prev = cur;
      cur = cur->next;
      return *this;
    }

    auto& operator*() { return cur; }
    const auto& operator*() const { return cur; }
    auto& operator->() { return cur; }
    const auto& operator->() const { return cur; }

    bool operator==(const InsertableIterator& other) const { return cur == other.cur; }

    bool operator!=(const InsertableIterator& other) const { return !(*this == other); }

  private:
    friend IntrusiveLinkedList;
    T* cur { nullptr };
    T* prev { nullptr };

    InsertableIterator() = default;
    InsertableIterator(T* curFront) : cur(curFront) { }
  };

public:
  ConstIterator begin() const {
    if (!mSize)
      return ConstIterator();
    return ConstIterator(mFront);
  }

  ConstIterator end() const { return ConstIterator(); }

  Iterator begin() {
    if (!mSize)
      return Iterator();
    return Iterator(mFront);
  }

  Iterator end() { return Iterator(); }

  template <typename T2>
  LockstepIteratorWrapper<T2> lockstepIterate(IntrusiveLinkedList<T2>& other) {
    return LockstepIteratorWrapper<T2>(*this, other);
  }

  template <typename T2>
  ConstLockstepIteratorWrapper<T2> lockstepIterate(const IntrusiveLinkedList<T2>& other) {
    return ConstLockstepIteratorWrapper<T2>(*this, other);
  }

  InsertableIterator beginInsertable() {
    if (!mSize)
      return InsertableIterator();
    return InsertableIterator(mFront);
  }

  InsertableIterator endInsertable() { return InsertableIterator(); }

  void replace(InsertableIterator& loc, T&& val) {
    if (!loc.cur)
      throw std::runtime_error("Attempted to replace a non-existent value!");
    val.next = loc.cur->next;
    if (!loc.prev) {
      *loc.cur = val;
    } else {
      auto* prev = loc.prev;
      *loc.cur = val;
      prev->next = loc.cur;
    }
  }

  void insertBefore(InsertableIterator& loc, T* val) {
    if (!loc.cur) {
      push_back(val);
    } else if (!loc.prev) {
      mSize++;
      val->next = mFront;
      mFront = val;
    } else {
      mSize++;
      val->next = loc.prev->next;
      loc.prev->next = val;
    }
    loc.prev = val;
  }
};

}
