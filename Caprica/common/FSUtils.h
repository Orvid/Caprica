#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include <common/CaselessStringComparer.h>
#include <common/identifier_ref.h>

namespace caprica { namespace FSUtils {

static constexpr inline char SEP = std::filesystem::path::preferred_separator;
std::string_view basenameAsRef(std::string_view file);
std::string_view extensionAsRef(std::string_view file);
std::string_view filenameAsRef(std::string_view file);
std::string_view parentPathAsRef(std::string_view file);
std::filesystem::path objectNameToPath(const std::string& objectName);
std::string pathToObjectName(const std::filesystem::path& path);

std::filesystem::path normalize(const std::filesystem::path& path);
std::string canonical(const std::string& path);
std::filesystem::path canonicalFS(const std::filesystem::path& path);

}}
