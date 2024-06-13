#pragma once
#include "pugixml.hpp"
#include <common/CaselessStringComparer.h>
#include <common/identifier_ref.h>
#include <common/parser/PapyrusProject.h>
namespace caprica {

class CapricaPPJParser {

  std::vector<std::pair<std::string, std::string>> variables;
  std::string substituteString(const char* text);
  std::string ParseString(const pugi::xml_node& node);
  std::string ParseString(const pugi::xml_attribute& node);
  IncludeBase ParseIncludeBase(const pugi::xml_node& node);
  bool ParseVariables(const pugi::xml_node& node);
  CommandList ParseCommandList(const pugi::xml_node& node);

public:
  PapyrusProject Parse(const std::string& path);
};
}
