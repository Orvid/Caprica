#include <common/CapricaUserFlagsDefinition.h>

#include <common/CapricaError.h>

namespace caprica {

void CapricaUserFlagsDefinition::registerUserFlag(const UserFlag& flag) {
  if (flagNameMap.count(flag.name))
    CapricaError::fatal(flag.location, "A flag by the name of '%s' was already defined!", flag.name.c_str());
  for (auto& f : userFlags) {
    if (f.bitIndex == flag.bitIndex)
      CapricaError::fatal(flag.location, "Another flag is already defined with bit index %i!", (int)flag.bitIndex);
  }
  userFlags.push_back(flag);
  userFlags.back().flagNum = userFlags.size() - 1;
  flagNameMap.insert({ flag.name, userFlags.size() - 1 });
}

const CapricaUserFlagsDefinition::UserFlag& CapricaUserFlagsDefinition::findFlag(const CapricaFileLocation& loc, const std::string& name) const {
  auto a = flagNameMap.find(name);
  if (a == flagNameMap.end())
    CapricaError::fatal(loc, "Unknown flag '%s'!", name.c_str());
  return userFlags[a->second];
}

const CapricaUserFlagsDefinition::UserFlag& CapricaUserFlagsDefinition::getFlag(size_t flagNum) const {
  return userFlags[flagNum];
}

}
