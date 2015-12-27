#pragma once

#include <string>
#include <type_traits>
#include <vector>

#include <pex/PexFile.h>
#include <pex/PexUserFlags.h>

namespace caprica { namespace papyrus {

enum class PapyrusUserFlags
{
  None = 0,
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

inline pex::PexUserFlags buildPexUserFlags(pex::PexFile* file, PapyrusUserFlags flags) {
  pex::PexUserFlags pexFlags;
  if ((flags & PapyrusUserFlags::Hidden) == PapyrusUserFlags::Hidden)
    pexFlags |= file->getUserFlag(file->getString("hidden"), 0);
  if ((flags & PapyrusUserFlags::Conditional) == PapyrusUserFlags::Conditional)
    pexFlags |= file->getUserFlag(file->getString("conditional"), 1);
  if ((flags & PapyrusUserFlags::Default) == PapyrusUserFlags::Default)
    pexFlags |= file->getUserFlag(file->getString("default"), 2);
  if ((flags & PapyrusUserFlags::CollapsedOnRef) == PapyrusUserFlags::CollapsedOnRef)
    pexFlags |= file->getUserFlag(file->getString("collapsedonref"), 3);
  if ((flags & PapyrusUserFlags::CollapsedOnBase) == PapyrusUserFlags::CollapsedOnBase)
    pexFlags |= file->getUserFlag(file->getString("collapsedonbase"), 4);
  if ((flags & PapyrusUserFlags::Mandatory) == PapyrusUserFlags::Mandatory)
    pexFlags |= file->getUserFlag(file->getString("mandatory"), 5);
  return pexFlags;
}

}}