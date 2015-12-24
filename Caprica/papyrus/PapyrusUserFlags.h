#pragma once

#include <string>
#include <type_traits>
#include <vector>

namespace caprica { namespace papyrus {

enum class PapyrusUserFlags
{
  None = 0,
  // Note that the actual values here are 1 higher than the
  // actual flag bit index. Shift these values right 1 to
  // get the actual flag value.
  Hidden          = 0b00000001,
  Conditional     = 0b00000010,
  Default         = 0b00000100,
  CollapsedOnRef  = 0b00001000,
  CollapsedOnBase = 0b00010000,
  Mandatory       = 0b00100000,
};

inline PapyrusUserFlags operator &(PapyrusUserFlags a, PapyrusUserFlags b) {
  return (PapyrusUserFlags)((std::underlying_type<PapyrusUserFlags>::type)a & (std::underlying_type<PapyrusUserFlags>::type)b);
}
inline PapyrusUserFlags operator |(PapyrusUserFlags a, PapyrusUserFlags b) {
  return (PapyrusUserFlags)((std::underlying_type<PapyrusUserFlags>::type)a | (std::underlying_type<PapyrusUserFlags>::type)b);
}
inline PapyrusUserFlags& operator &=(PapyrusUserFlags& a, PapyrusUserFlags b) {
  return a = a & b;
}
inline PapyrusUserFlags& operator |=(PapyrusUserFlags& a, PapyrusUserFlags b) {
  return a = a | b;
}

}}