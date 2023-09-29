#include "FakeScripts.h"
#include <common/GameID.h>

namespace caprica {
constexpr const char* FAKE_SKYRIM_SCRIPTOBJECT_SCRIPT =
    R"(Scriptname __ScriptObject Hidden

String Function GetState()
  { Function that returns the current state }
  Return __state
EndFunction

Function GotoState(String newState)
  { Function that switches this object to the specified state }
  Self.onEndState()
  __state = newState
  Self.onBeginState()
EndFunction

Event OnInit()
  ; empty
EndEvent

Event OnBeginState()
  { Event received when this state is switched to }
EndEvent

Event onEndState()
  { Event received when this state is switched away from }
EndEvent
)";

// This was not included in the source release for skyrim, and it is depended on by others.
constexpr const char* MISSING_DLC1SCWispWallScript_SKYRIM =
    R"(ScriptName DLC1SCWispWallScript Extends ObjectReference

;-- Variables ---------------------------------------
Int DeadCount = 0
Int WispCount = 0

;-- Properties --------------------------------------
Keyword Property DLC1WispWallEffect Auto
Quest Property QuestToSetOnWallDown Auto
Int Property StageToSet Auto

;-- Functions ---------------------------------------

Function GetWispNumber()
  WispCount += 1
EndFunction


Function CheckDead()
  DeadCount += 1
  If DeadCount == WispCount
    Self.DropWall(Self.GetLinkedRef(DLC1WispWallEffect))
    If QuestToSetOnWallDown
      QuestToSetOnWallDown.SetStage(StageToSet)
    EndIf
  EndIf
EndFunction

Function DropWall(ObjectReference WallEffect)
  WallEffect.disable(True)
  Self.disable(False)
EndFunction
)";

constexpr const char* STARFIELD_FAKE_FLAGS_FILE =
R"(// Starfield flags file, by NikitaLita
// This file is conjecture, based on the userflags set by the base starfield game scripts.

// Flag hides the script or property from the game editor
        Flag Hidden 0
        {
          Script
                  Property
          StructVar
        }

// Flag on an object designates it as the script the condition system will look at
// Flag on a variable allows the script variable to be examined by the condition system
        Flag Conditional 1
        {
          Script
                  Variable
        }

// Flags the specified script as a default script, for filtering in the editor
        Flag Default 2
        {
          Script
        }

// Flags this group as collapsed on the reference in the editor
        Flag CollapsedOnRef 3
        {
          Group
        }

// Flags this group as collapsed on the base object in the editor
        Flag CollapsedOnBase 4
        {
          Group
        }

// Flags this group as collapsed (on both ref and base) in the editor
        Flag Collapsed CollapsedOnRef & CollapsedOnBase

// Flags a property as mandatory - in other words, will spit out a warning in
// the editor if it isn't given a value
        Flag Mandatory 5
        {
          Property
        }
)";



static const caseless_unordered_identifier_ref_map<identifier_ref> FAKE_SCRIPTS = {
  {"fake://skyrim/__ScriptObject.psc",        FAKE_SKYRIM_SCRIPTOBJECT_SCRIPT    },
  { "fake://skyrim/DLC1SCWispWallScript.psc", MISSING_DLC1SCWispWallScript_SKYRIM},
};

identifier_ref FakeScripts::getFakeScript(const identifier_ref& name, GameID game) {
  if (game != GameID::Skyrim)
    return identifier_ref();
  auto it = FAKE_SCRIPTS.find(name);
  if (it != FAKE_SCRIPTS.end())
    return it->second;
  return identifier_ref();
}
size_t FakeScripts::getSizeOfFakeScript(const identifier_ref& name, GameID game) {
  if (game != GameID::Skyrim)
    return 0;
  auto it = FAKE_SCRIPTS.find(name);
  if (it != FAKE_SCRIPTS.end())
    return 0;
  return it->second.size();
}

identifier_ref FakeScripts::getFakeFlagsFile(GameID game) {
  if (game != GameID::Starfield)
    return {};
  return STARFIELD_FAKE_FLAGS_FILE;
}


}
