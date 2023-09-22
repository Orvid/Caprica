#pragma once

namespace caprica {

struct CapricaReferenceState final {
  bool isInitialized { false };
  bool isRead { false };
  bool isWritten { false };
};

}
