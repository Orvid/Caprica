#pragma once

#include <type_traits>

namespace caprica { namespace allocators {

template <typename T>
struct CachePool final {
  static_assert(std::is_same<decltype(&T::reset), void (T::*)()>::value, "T must be resettable.");

  T* acquire() {
    Node* n = nullptr;
    if (freeValueList != nullptr) {
      n = freeValueList;
      freeValueList = n->next;
      auto val = n->value;
      n->value = nullptr;
      n->next = freeNodeList;
      freeNodeList = n;
      val->reset();
      return val;
    }
    return new T();
  }

  void release(T* val) {
    Node* n = nullptr;
    if (freeNodeList != nullptr) {
      n = freeNodeList;
      freeNodeList = freeNodeList->next;
    } else {
      n = new Node();
    }
    n->value = val;
    n->next = freeValueList;
    freeValueList = n;
  }

private:
  struct Node final {
    T* value { nullptr };
    Node* next { nullptr };
  };

  Node* freeNodeList { nullptr };
  Node* freeValueList { nullptr };
};

}}
