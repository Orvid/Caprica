#pragma once
#include <common/CaselessStringComparer.h>
#include <common/GameID.h>


namespace caprica {
struct CapricaUserFlagsDefinition;
struct FakeScripts {
  static identifier_ref getFakeScript(const identifier_ref& name, GameID game);
  static size_t getSizeOfFakeScript(const identifier_ref& name, GameID game);
  static identifier_ref getFakeFlagsFile(GameID game);
};
}
