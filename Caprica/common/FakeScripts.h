#pragma once
#include "CaselessStringComparer.h"
#include "GameID.h"

namespace caprica {
struct FakeScripts{
static identifier_ref getFakeScript(const identifier_ref& name, GameID game);
static size_t getSizeOfFakeScript(const identifier_ref& name, GameID game);
};
}
