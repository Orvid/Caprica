#pragma once

#include <string>

#include <common/CaselessStringComparer.h>

namespace caprica { namespace papyrus {

struct PapyrusNamespaceResolutionContext final
{
  static void pushNamespaceFullContents(const std::string& namespaceName, caseless_unordered_identifier_map<std::string>&& map);
  static bool tryFindType(const std::string& baseNamespace,
                          const std::string& typeName,
                          std::string* retFullTypeName,
                          std::string* retFullTypePath,
                          std::string* retStructName);
};

}}
