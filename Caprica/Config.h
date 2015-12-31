#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace caprica { namespace Config {

bool enableLanguageExtensions{ false };
bool enableSpeculativeSyntax{ true };
std::vector<std::string> importDirectories{ };

}}