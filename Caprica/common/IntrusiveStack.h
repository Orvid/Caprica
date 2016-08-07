#pragma once

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
  }

  void pop() {
    T* tmp;
    tryPop(tmp);
  }

  T* top() { return root; }
  const T* top() const { return root; }

  bool tryPop(T*& ret) {
    if (!root)
      return false;

    ret = root;
    root = root->nextInStack;
    return true;
  }



private:
  T* root{ nullptr };
};

}
