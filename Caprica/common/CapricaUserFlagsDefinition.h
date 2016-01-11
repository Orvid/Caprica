#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

#include <common/CapricaFileLocation.h>
#include <common/CaselessStringComparer.h>

namespace caprica {

struct CapricaUserFlagsDefinition final
{
  enum class ValidLocations
  {
    None          = 0b00000000,
    Function      = 0b00000001,
    Property      = 0b00000010,
    PropertyGroup = 0b00000100,
    Script        = 0b00001000,
    StructMember  = 0b00010000,
    Variable      = 0b00100000,

    AllLocations = Function | Property | PropertyGroup | Script | StructMember | Variable,
  };

  struct UserFlag final
  {
    ValidLocations validLocations{ ValidLocations::None };
    std::string name{ "" };
    uint8_t bitIndex{ };
    size_t flagNum{ };
    CapricaFileLocation location;

    UserFlag(const CapricaFileLocation& loc) : location(loc) { }
    ~UserFlag() = default;
  };

  void registerUserFlag(const UserFlag& flag);
  const UserFlag& findFlag(const CapricaFileLocation& loc, const std::string& name) const;
  // Not that flag num is NOT the flag's bit index, it is instead
  // the flag's index in the user flags vector.
  const UserFlag& getFlag(size_t flagNum) const;

private:
  std::map<std::string, size_t, CaselessStringComparer> flagNameMap{ };
  std::vector<UserFlag> userFlags{ };
};

inline auto operator ~(CapricaUserFlagsDefinition::ValidLocations a) {
  return (decltype(a))(~(std::underlying_type<decltype(a)>::type)a);
}
inline auto operator &(CapricaUserFlagsDefinition::ValidLocations a, CapricaUserFlagsDefinition::ValidLocations b) {
  return (decltype(a))((std::underlying_type<decltype(a)>::type)a & (std::underlying_type<decltype(b)>::type)b);
}
inline auto operator |(CapricaUserFlagsDefinition::ValidLocations a, CapricaUserFlagsDefinition::ValidLocations b) {
  return (decltype(a))((std::underlying_type<decltype(a)>::type)a | (std::underlying_type<decltype(b)>::type)b);
}
inline auto& operator &=(CapricaUserFlagsDefinition::ValidLocations& a, CapricaUserFlagsDefinition::ValidLocations b) {
  return a = a & b;
}
inline auto& operator |=(CapricaUserFlagsDefinition::ValidLocations& a, CapricaUserFlagsDefinition::ValidLocations b) {
  return a = a | b;
}

}
