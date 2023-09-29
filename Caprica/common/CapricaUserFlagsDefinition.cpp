#include <common/CapricaUserFlagsDefinition.h>

namespace caprica {

size_t CapricaUserFlagsDefinition::UserFlag::getData() const {
  if (flagData)
    return flagData;
  return 1ULL << flagNum;
}

bool CapricaUserFlagsDefinition::UserFlag::isValidOn(ValidLocations loc) const {
  if ((validLocations & loc) == loc)
    return true;
  if (loc != ValidLocations::Property)
    return false;
  if ((validLocations & ValidLocations::Variable) != ValidLocations::Variable)
    return false;
  return true;
}

void CapricaUserFlagsDefinition::registerUserFlag(CapricaReportingContext& repCtx, const UserFlag& flag) {
  if (flagNameMap.count(flag.name)) {
    repCtx.error(flag.location, "A flag by the name of '{}' was already defined!", flag.name);
    return;
  }
  for (auto& f : userFlags) {
    if (f.bitIndex == flag.bitIndex && !f.flagData && !flag.flagData) {
      repCtx.error(flag.location, "Another flag is already defined with bit index {}!", (int)flag.bitIndex);
      return;
    }
  }
  userFlags.push_back(flag);
  userFlags.back().flagNum = userFlags.size() - 1;
  flagNameMap.insert({ flag.name, userFlags.size() - 1 });
}

const CapricaUserFlagsDefinition::UserFlag& CapricaUserFlagsDefinition::findFlag(CapricaReportingContext& repCtx,
                                                                                 CapricaFileLocation loc,
                                                                                 identifier_ref name) const {
  auto a = flagNameMap.find(name);
  if (a == flagNameMap.end()) {
    repCtx.error(loc, "Unknown flag '{}'!", name);
    if (!flagNameMap.empty())
      return userFlags.front();
    repCtx.fatal(loc, "No user flags have been defined.");
  }
  return userFlags[a->second];
}

const CapricaUserFlagsDefinition::UserFlag& CapricaUserFlagsDefinition::getFlag(size_t flagNum) const {
  return userFlags[flagNum];
}

}
