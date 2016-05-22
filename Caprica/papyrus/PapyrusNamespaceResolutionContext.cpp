#include <papyrus/PapyrusNamespaceResolutionContext.h>

#include <common/CapricaReportingContext.h>
#include <common/Concurrency.h>

namespace caprica { namespace papyrus {

namespace {

struct PapyrusNamespace final
{
  std::string fullName{ "" };
  std::string name{ "" };
  PapyrusNamespace* parent{ nullptr };
  caseless_concurrent_unordered_identifier_map<std::string, PapyrusNamespace*> children{ };
  // Key is unqualified name, value is full path to file.
  caseless_unordered_identifier_map<std::string, std::string> objects{ };

  void createNamespace(const std::string& curPiece, caseless_unordered_identifier_map<std::string, std::string>&& map) {
    if (curPiece == "") {
      objects = std::move(map);
      return;
    }

    std::string curSearchPiece = curPiece;
    std::string nextSearchPiece = "";
    auto loc = curPiece.find(':');
    if (loc != std::string::npos) {
      curSearchPiece = curPiece.substr(0, loc);
      nextSearchPiece = curPiece.substr(loc + 1);
    }

    auto f = children.find(curSearchPiece);
    if (f == children.end()) {
      auto n = new PapyrusNamespace();
      n->name = curSearchPiece;
      n->fullName = curSearchPiece;
      if (fullName != "")
        n->fullName = fullName + ':' + curSearchPiece;
      n->parent = this;
      children.insert({ curSearchPiece, n });
      f = children.find(curSearchPiece);
    }
    f->second->createNamespace(nextSearchPiece, std::move(map));
  }

  bool tryFindNamespace(const std::string& curPiece, PapyrusNamespace const** ret) const {
    if (curPiece == "") {
      *ret = this;
      return true;
    }

    std::string curSearchPiece = curPiece;
    std::string nextSearchPiece = "";
    auto loc = curPiece.find(':');
    if (loc != std::string::npos) {
      curSearchPiece = curPiece.substr(0, loc);
      nextSearchPiece = curPiece.substr(loc + 1);
    }

    auto f = children.find(curSearchPiece);
    if (f != children.end())
      return f->second->tryFindNamespace(nextSearchPiece, ret);

    return false;
  }

  bool tryFindType(const std::string& typeName, std::string* retTypeName, std::string* retTypePath, std::string* retStructName) const {
    auto loc = typeName.find(':');
    if (loc == std::string::npos) {
      if (objects.find(typeName) != objects.end()) {
        if (fullName != "")
          *retTypeName = fullName + ':' + typeName;
        else
          *retTypeName = typeName;
        *retTypePath = objects.at(typeName);
        return true;
      }
      return false;
    }

    // It's a partially qualified type name, or else is referencing
    // a struct.
    auto baseName = typeName.substr(0, loc);
    auto subName = typeName.substr(loc + 1);

    // It's a partially qualified name.
    auto f = children.find(baseName);
    if (f != children.end())
      return f->second->tryFindType(subName, retTypeName, retTypePath, retStructName);

    // subName is still partially qualified, so it can't
    // be referencing a struct in this namespace.
    if (subName.find(':') != std::string::npos)
      return false;

    // It is a struct reference.
    if (objects.find(baseName) != objects.end()) {
      if (fullName != "")
        *retTypeName = fullName + ':' + baseName;
      else
        *retTypeName = baseName;
      *retTypePath = objects.at(baseName);
      *retStructName = subName;
      return true;
    }

    return false;
  }
};
}

static PapyrusNamespace rootNamespace{ };
void PapyrusNamespaceResolutionContext::pushNamespaceFullContents(const std::string& namespaceName, caseless_unordered_identifier_map<std::string, std::string>&& map) {
  rootNamespace.createNamespace(namespaceName, std::move(map));
}

bool PapyrusNamespaceResolutionContext::tryFindType(const std::string& baseNamespace,
                                                    const std::string& typeName,
                                                    std::string* retFullTypeName,
                                                    std::string* retFullTypePath,
                                                    std::string* retStructName) {
  const PapyrusNamespace* curNamespace = nullptr;
  if (!rootNamespace.tryFindNamespace(baseNamespace, &curNamespace))
    return false;

  while (curNamespace != nullptr) {
    if (curNamespace->tryFindType(typeName, retFullTypeName, retFullTypePath, retStructName))
      return true;
    curNamespace = curNamespace->parent;
  }
  return false;
}

}}
