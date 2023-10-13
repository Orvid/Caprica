#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include <common/CaselessStringComparer.h>
#include <common/identifier_ref.h>

namespace caprica { namespace FSUtils {

static constexpr char SEP = std::filesystem::path::preferred_separator;
std::string_view basenameAsRef(std::string_view file);
std::string_view extensionAsRef(std::string_view file);
std::string_view filenameAsRef(std::string_view file);
std::string_view parentPathAsRef(std::string_view file);

std::string canonical(const std::string& path);
std::filesystem::path canonicalFS(const std::filesystem::path& path);

}}
