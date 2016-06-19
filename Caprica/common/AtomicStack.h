#pragma once

#include <atomic>

namespace caprica {

template<typename T>
struct AtomicStack final
{
  struct Node
  {
    std::atomic<T*> next{ nullptr };
  };

  bool tryPop(T*& ret) {
    auto n = root.load();
    if (!n)
      return false;

    auto newN = n->next.load();
    while (!root.compare_exchange_weak(n, newN)) {
      if (!n) {
        return false;
      }
      newN = n->next.load();
    }

    ret = n;
    return true;
  }

  void push(T* node) {
    auto curValue = root.load(std::memory_order_acquire);
    do {
      node->next = curValue;
    } while (!root.compare_exchange_weak(curValue, node));
  }

private:
  std::atomic<T*> root{ nullptr };
};

}
