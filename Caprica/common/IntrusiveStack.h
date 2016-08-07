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
  size_t count{ 0 };
  T* root{ nullptr };
};

}
