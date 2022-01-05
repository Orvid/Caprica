#pragma once

#include <cstring>
#include <string>
#include <string_view>

#include <common/identifier_ref.h>

namespace caprica {
struct LargelyBufferedString {
  char buf[256];
  size_t bufLen{ 0 };
  std::string* oolBuf{ nullptr };

  LargelyBufferedString() = default;
  ~LargelyBufferedString() {
    if (oolBuf) {
      delete oolBuf;
    }
  }

  LargelyBufferedString(std::string_view view) {
    this->append(view);
  }

  LargelyBufferedString& append(std::string_view view) {
    if (oolBuf != nullptr) {
      oolBuf->append(view);
    } else {
      if (view.size() + bufLen >= sizeof(buf)) {
        oolBuf = new std::string(buf, bufLen);
        oolBuf->append(view);
      } else {
        memcpy(&buf[bufLen], view.data(), view.size());
        bufLen += view.size();
      }
    }
    return *this;
  }

  void push_back(char c) {
    this->append(std::string_view(&c, 1));
  }

  std::string_view string_view() {
    return std::string_view(data(), size());
  }

  char* data() {
    if (oolBuf)
      return oolBuf->data();
    else
      return buf;
  }

  size_t size() const {
    if (oolBuf)
      return oolBuf->size();
    else
      return bufLen;
  }
};
}