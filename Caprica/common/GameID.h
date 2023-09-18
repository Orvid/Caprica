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

constexpr const char *GameIDToString(GameID game) {
  switch (game) {
    case GameID::INVALID:
      return "INVALID";
    case GameID::UNKNOWN:
      return "UNKNOWN";
    case GameID::Skyrim:
      return "Skyrim";
    case GameID::Fallout4:
      return "Fallout4";
    case GameID::Fallout76:
      return "Fallout76";
    case GameID::Starfield:
      return "Starfield";
    default:
      return "INVALID";
  }

};
}