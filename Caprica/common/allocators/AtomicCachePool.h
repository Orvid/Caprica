#pragma once

#include <atomic>
#include <type_traits>

#include <common/AtomicStack.h>

namespace caprica { namespace allocators {

template<typename T>
struct AtomicCachePool final
{
  static_assert(std::is_same<decltype(&T::reset), void(T::*)()>::value, "T must be resettable.");

  T* acquire() {
    Node* n = nullptr;
    if (freeValueList.tryPop(n)) {
      auto val = n->value;
      n->value = nullptr;
      freeNodeList.push(n);
      val->reset();
      return val;
    }
    return new T();
  }

  void release(T* val) {
    Node* n = nullptr;
    if (!freeNodeList.tryPop(n))
      n = new Node();
    n->value = val;
    freeValueList.push(n);
  }

private:
  struct Node final : AtomicStack<Node>::Node
  {
    T* value{ nullptr };
  };

  AtomicStack<Node> freeNodeList{ };
  AtomicStack<Node> freeValueList{ };
};

}}
