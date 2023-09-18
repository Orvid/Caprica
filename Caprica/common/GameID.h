#pragma once
#include <cstdint>

namespace caprica {


// matches values in "GameID" field in PEX header
enum class GameID : uint16_t {
  INVALID = 0xFFFF,
  UNKNOWN = 0,
  Skyrim = 1,
  Fallout4 = 2,
  Fallout76 = 3,
  Starfield = 4,
};

}
