#pragma once

#include <cstdlib>

namespace caprica {

// This intentionally uses a different member name
// from IntrusiveLinkedList so that things can be
// in both an IntrusiveLinkedList and an IntrusiveStack
// at the same time.
template<typename T>
struct IntrusiveStack final
{
  void push(T* node) {
    node->nextInStack = root;
    root = node;
    count++;
  }

  T* pop() {
    T* ret = root;
    root = root->nextInStack;
    count--;
    return ret;
  }

  T* top() { return root; }
  const T* top() const { return root; }

  bool tryPop(T*& ret) {
    if (!root)
      return false;

    count--;
    ret = root;
    root = root->nextInStack;
    return true;
  }

  size_t size() const { return count; }

private:
  struct ConstIterator final
  {
    ConstIterator& operator ++() {
      if (cur == nullptr)
        return *this;
      cur = cur->nextInStack;
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
    friend IntrusiveStack;
    const T* cur{ nullptr };

    ConstIterator() = default;
    ConstIterator(const T* mFront) : cur(mFront) { }
  };

  struct Iterator final
  {
    Iterator& operator ++() {
      if (cur == nullptr)
        return *this;
      cur = cur->nextInStack;
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
    friend IntrusiveStack;
    T* cur{ nullptr };

    Iterator() = default;
    Iterator(T* mFront) : cur(mFront) { }
  };

  size_t count{ 0 };
  T* root{ nullptr };

public:
  ConstIterator begin() const {
    if (!count)
      return ConstIterator();
    return ConstIterator(root);
  }

  ConstIterator end() const {
    return ConstIterator();
  }

  Iterator begin() {
    if (!count)
      return Iterator();
    return Iterator(root);
  }

  Iterator end() {
    return Iterator();
  }
};

}
