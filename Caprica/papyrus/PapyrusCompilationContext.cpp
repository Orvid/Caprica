#include <papyrus/PapyrusCompilationContext.h>

#include <fcntl.h>
#include <filesystem>
#include <io.h>
#include <iostream>

#include <common/allocators/AtomicChainedPool.h>
#include <common/CapricaConfig.h>
#include <common/FakeScripts.h>

#include <papyrus/parser/PapyrusParser.h>

#include <pex/parser/PexAsmParser.h>
#include <pex/PexOptimizer.h>
#include <pex/PexReflector.h>

namespace caprica { namespace papyrus {

void PapyrusCompilationNode::awaitRead() {
  readJob.await();
}

std::string PapyrusCompilationNode::awaitPreParse() {
  preParseJob.await();
  return objectName;
}


PapyrusScript* PapyrusCompilationNode::awaitParse() {
  parseJob.await();
  return loadedScript;
}

PapyrusObject* PapyrusCompilationNode::awaitPreSemantic() {
  preSemanticJob.await();
  return resolvedObject;
}

PapyrusObject* PapyrusCompilationNode::awaitSemantic() {
  semanticJob.await();
  return resolvedObject;
}

void PapyrusCompilationNode::queueCompile() {
  switch (type) {
    case NodeType::PapyrusImport:
    case NodeType::PasReflection:
    case NodeType::PexReflection:
      return;
  }
  jobManager->queueJob(&writeJob);
}

void PapyrusCompilationNode::awaitWrite() {
  switch (type) {
    case NodeType::PapyrusImport:
    case NodeType::PexDissassembly: // NOTE: Pex disassembly gets written during Compile, not during Write?
    case NodeType::PasReflection:
    case NodeType::PexReflection:
      return;
  }
  writeJob.await();
}

PapyrusCompilationNode::NodeType PapyrusCompilationNode::getType() const {
  // TODO: This probably isn't thread safe
  return type;
}

allocators::AtomicChainedPool readAllocator { 1024 * 1024 * 4 };
void PapyrusCompilationNode::FileReadJob::run() {
  if (parent->type == NodeType::PapyrusCompile || parent->type == NodeType::PasCompile ||
      parent->type == NodeType::PexDissassembly) {
    if (!conf::General::quietCompile)
      std::cout << "Compiling " << parent->reportedName << std::endl;
  }
  // TODO: remove this hack when imports are working
  if (parent->sourceFilePath.starts_with("fake://")) {
    parent->ownedReadFileData =
        std::move(FakeScripts::getFakeScript(parent->sourceFilePath, conf::Papyrus::game).to_string());
    parent->readFileData = parent->ownedReadFileData;
    return;
  }
  if (parent->filesize < std::numeric_limits<uint32_t>::max()) {
    auto buf = readAllocator.allocate(parent->filesize + 1);
    auto fd = _open(parent->sourceFilePath.c_str(), _O_BINARY | _O_RDONLY | _O_SEQUENTIAL);
    if (fd != -1) {
      auto len = _read(fd, (void*)buf, (uint32_t)parent->filesize);
      parent->readFileData = std::string_view(buf, len);
      if (_eof(fd) == 1) {
        _close(fd);
        // Need this to be null terminated.
        buf[parent->filesize] = '\0';
        return;
      }
      _close(fd);
    }
  }
  {
    std::string str;
    str.resize(parent->filesize);
    std::ifstream inFile { parent->sourceFilePath, std::ifstream::binary };
    inFile.exceptions(std::ifstream::badbit | std::ifstream::failbit);
    if (parent->filesize != 0)
      inFile.read((char*)str.data(), parent->filesize);
    // Just because the filesize was one thing when
    // we iterated the directory doesn't mean it's
    // not gotten bigger since then.
    inFile.peek();
    if (!inFile.eof()) {
      std::stringstream strStream {};
      strStream.exceptions(std::ifstream::badbit | std::ifstream::failbit);
      strStream << inFile.rdbuf();
      str += strStream.str();
    }
    str += '\0';
    parent->ownedReadFileData = std::move(str);
    parent->readFileData = parent->ownedReadFileData;
  }
}

std::string_view findScriptName(const std::string_view& data, const std::string_view& startstring, bool stripWhitespace = false)
{
  size_t last = 0;
  while (last < data.size()) {
    auto next = data.find('\n', last);
    if (next == std::string_view::npos)
      next = data.size() - 1;
    auto line = data.substr(last, next - last);
    auto begin = stripWhitespace ? line.find_first_not_of(" \t") : 0;
    if (strnicmp(line.substr(begin, startstring.size()).data(), startstring.data(), startstring.size()) == 0) {
      auto first = line.find_first_not_of(" \t", startstring.size());
      return line.substr(first, line.find_first_of(" \t", first) - first);
    }
    last = next + 1;
  }
  return {};
}

void PapyrusCompilationNode::FilePreParseJob::run() {
  parent->readJob.await();
  auto ext = FSUtils::extensionAsRef(parent->sourceFilePath);
  if (pathEq(ext, ".psc")) {
    parent->objectName = findScriptName(parent->readFileData, "scriptname");
  } else if (pathEq(ext, ".pex")) {
    // have to read the whole pexfile in order to get the script name
    pex::PexReader rdr(parent->sourceFilePath);
    auto alloc = new allocators::ChainedPool(1024 * 4);
    parent->pexFile = pex::PexFile::read(alloc, rdr);
    parent->isPexFile = true;
    if (parent->pexFile->objects.size() == 0) {
      CapricaReportingContext::logicalFatal("Unable to find script name in '%s'.", parent->sourceFilePath.c_str());
    }
    parent->objectName = parent->pexFile->getStringValue(parent->pexFile->objects.front()->name).to_string();
  } else if (pathEq(ext, ".pas")) {
    parent->objectName = findScriptName(parent->readFileData, ".object", true);
  } else {
    CapricaReportingContext::logicalFatal("Unable to determine the type of file to load '%s' as.",
                                          parent->reportedName.c_str());
  }
  if (parent->objectName.empty()){
    CapricaReportingContext::logicalFatal("Unable to find script name in '%s'.", parent->sourceFilePath.c_str());
  }
}

void PapyrusCompilationNode::FileParseJob::run() {
  parent->preParseJob.await();
  auto ext = FSUtils::extensionAsRef(parent->sourceFilePath);
  if (pathEq(ext, ".psc")) {
    auto parser = new parser::PapyrusParser(parent->reportingContext, parent->sourceFilePath, parent->readFileData);
    parent->loadedScript = parser->parseScript();
    if (parent->type != NodeType::PapyrusImport)
      parent->reportingContext.exitIfErrors();
    delete parser;
  } else if (pathEq(ext, ".pex")) {
    // nothing to do here
  } else if (pathEq(ext, ".pas")) {
    auto parser = new pex::parser::PexAsmParser(parent->reportingContext, parent->sourceFilePath);
    parent->pexFile = parser->parseFile();
    parent->reportingContext.exitIfErrors();
    delete parser;
    parent->isPexFile = true;
    if (parent->type == NodeType::PasCompile)
      return;
  } else {
    CapricaReportingContext::logicalFatal("Unable to determine the type of file to load '%s' as.",
                                          parent->reportedName.c_str());
  }

  if (parent->pexFile) {
    parent->loadedScript = pex::PexReflector::reflectScript(parent->pexFile);
    parent->reportingContext.exitIfErrors();
    delete parent->pexFile->alloc;
    parent->pexFile = nullptr;
  }

  assert(parent->loadedScript != nullptr);
  if (parent->loadedScript->objects.size() != 1)
    CapricaReportingContext::logicalFatal("The script had either no objects or more than one!");
}

void PapyrusCompilationNode::FilePreSemanticJob::run() {
  parent->parseJob.await();
  for (auto o : parent->loadedScript->objects)
    o->compilationNode = parent;
  parent->resolutionContext = new PapyrusResolutionContext(parent->reportingContext);
  parent->resolutionContext->allocator = parent->loadedScript->allocator;
  parent->resolutionContext->isPexResolution = parent->isPexFile;
  parent->loadedScript->preSemantic(parent->resolutionContext);
  parent->reportingContext.exitIfErrors();
  parent->resolvedObject = parent->loadedScript->objects.front();
}

void PapyrusCompilationNode::FileSemanticJob::run() {
  parent->preSemanticJob.await();
  parent->loadedScript->semantic(parent->resolutionContext);
  if (parent->type != NodeType::PapyrusImport)
    parent->reportingContext.exitIfErrors();
}

static constexpr bool disablePexBuild = false;

void PapyrusCompilationNode::FileCompileJob::run() {
  parent->semanticJob.await();
  switch (parent->type) {
    case NodeType::PapyrusCompile: {
      parent->loadedScript->semantic2(parent->resolutionContext);
      parent->reportingContext.exitIfErrors();
      delete parent->resolutionContext;
      parent->resolutionContext = nullptr;

      if (!disablePexBuild) {
        parent->pexFile = parent->loadedScript->buildPex(parent->reportingContext);
        parent->reportingContext.exitIfErrors();

        if (conf::CodeGeneration::enableOptimizations)
          pex::PexOptimizer::optimize(parent->pexFile);

        parent->pexWriter = new pex::PexWriter();
        parent->pexFile->write(*parent->pexWriter);

        if (conf::Debug::dumpPexAsm) {
          auto baseFileName = std::string(FSUtils::basenameAsRef(parent->sourceFilePath));
          auto containingDir = std::filesystem::path(parent->outputDirectory);
          if (!std::filesystem::exists(containingDir))
            std::filesystem::create_directories(containingDir);
          std::ofstream asmStrm(parent->outputDirectory + "\\" + std::string(parent->baseName) + ".pas",
                                std::ofstream::binary);
          asmStrm.exceptions(std::ifstream::badbit | std::ifstream::failbit);
          pex::PexAsmWriter asmWtr(asmStrm);
          parent->pexFile->writeAsm(asmWtr);
        }

        delete parent->pexFile->alloc;
        parent->pexFile = nullptr;
      }
      return;
    }
    case NodeType::PexDissassembly: {
      auto baseFileName = std::string(FSUtils::basenameAsRef(parent->sourceFilePath));
      auto containingDir = std::filesystem::path(parent->outputDirectory);
      if (!std::filesystem::exists(containingDir))
        std::filesystem::create_directories(containingDir);
      std::ofstream asmStrm(parent->outputDirectory + "\\" + std::string(parent->baseName) + ".pas",
                            std::ofstream::binary);
      asmStrm.exceptions(std::ifstream::badbit | std::ifstream::failbit);
      caprica::pex::PexAsmWriter asmWtr(asmStrm);
      parent->pexFile->writeAsm(asmWtr);
      delete parent->pexFile->alloc;
      parent->pexFile = nullptr;
      return;
    }
    case NodeType::PasCompile: {
      if (conf::CodeGeneration::enableOptimizations)
        pex::PexOptimizer::optimize(parent->pexFile);

      parent->pexWriter = new pex::PexWriter();
      parent->pexFile->write(*parent->pexWriter);
      delete parent->pexFile->alloc;
      parent->pexFile = nullptr;
      return;
    }

    // TODO: fix this hack
    case NodeType::PapyrusImport:
    case NodeType::PasReflection:
    case NodeType::PexReflection:
      return;
    case NodeType::Unknown:
    default:
      break;
  }
  CapricaReportingContext::logicalFatal("You shouldn't be trying to compile this!");
}

void PapyrusCompilationNode::FileWriteJob::run() {
  parent->compileJob.await();
  switch (parent->type) {
    case NodeType::PasCompile:
    case NodeType::PapyrusCompile: {
      if (!conf::Performance::performanceTestMode) {
        auto baseFileName = std::string(FSUtils::basenameAsRef(parent->sourceFilePath));
        auto containingDir = std::filesystem::path(parent->outputDirectory);
        if (!std::filesystem::exists(containingDir))
          std::filesystem::create_directories(containingDir);
        std::ofstream destFile { parent->outputDirectory + "\\" + baseFileName + ".pex", std::ifstream::binary };
        destFile.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        parent->pexWriter->applyToBuffers([&](const char* data, size_t size) { destFile.write(data, size); });
      }
      delete parent->pexWriter;
      parent->pexWriter = nullptr;
      return;
    }
    // TODO: remove this hack
    case NodeType::PapyrusImport:
    case NodeType::PexDissassembly:
    case NodeType::PasReflection:
    case NodeType::PexReflection:
      return;
    case NodeType::Unknown:
    default:
      break;
  }
  CapricaReportingContext::logicalFatal("You shouldn't be trying to compile this!");
}

namespace {
static std::vector<PapyrusCompilationNode*> nodesToCleanUp {};
struct PapyrusNamespace final {
  std::string name { "" };
  PapyrusNamespace* parent { nullptr };
  caseless_unordered_identifier_ref_map<PapyrusNamespace*> children {};
  // Key is unqualified name, value is full path to file.
  caseless_unordered_identifier_ref_map<PapyrusCompilationNode*> objects {};

  void awaitRead() {
    for (auto o : objects)
      o.second->awaitRead();
    for (auto c : children)
      c.second->awaitRead();
  }

  void awaitPreParse() {
    for (auto o : objects)
      o.second->awaitPreParse();
    for (auto c : children)
      c.second->awaitPreParse();
  }

  void awaitParse() {
    for (auto o : objects)
      o.second->awaitParse();
    for (auto c : children)
      c.second->awaitParse();
  }

  void awaitPreSemantic() {
    for (auto o : objects)
      o.second->awaitPreSemantic();
    for (auto c : children)
      c.second->awaitPreSemantic();
  }

  void queueCompile() {
    for (auto o : objects)
      o.second->queueCompile();
    for (auto c : children)
      c.second->queueCompile();
  }

  void awaitCompile() {
    for (auto o : objects)
      o.second->awaitWrite();
    for (auto c : children)
      c.second->awaitCompile();
  }

  void createNamespace(const identifier_ref& curPiece,
                       caseless_unordered_identifier_ref_map<PapyrusCompilationNode*>&& map) {
    if (conf::Papyrus::game == GameID::Skyrim && curPiece != "")
      CapricaReportingContext::logicalFatal("Invalid namespacing on Skyrim script: %s", curPiece.to_string().c_str());
    if (curPiece == "") {
      if (!objects.empty()) {
        // we have to merge them
        for (auto& obj : map) {
          auto f = objects.find(obj.first);
          if (f != objects.end()) {
            // we have a duplicate; we usually ignore this, but if the old object is just an import and the new object
            // is a compile node, we have to replace it
            // TODO: This will create issues later when we allow for reloads and such, but for now it's fine, we do this
            // before we actually get a resolved object to reference
            switch (f->second->getType()) {
              case PapyrusCompilationNode::NodeType::PapyrusImport:
              case PapyrusCompilationNode::NodeType::PasReflection:
              case PapyrusCompilationNode::NodeType::PexReflection:
                switch (obj.second->getType()) {
                  case PapyrusCompilationNode::NodeType::PapyrusCompile:
                  case PapyrusCompilationNode::NodeType::PasCompile:
                  case PapyrusCompilationNode::NodeType::PexDissassembly:
                    nodesToCleanUp.push_back(f->second);
                    f->second = obj.second;
                    break;
                  default:
                    break;
                }
                break;
              default:
                break;
            }
          } else {
            // we don't have a duplicate, so we can just add it
            objects.emplace(std::move(obj.first), std::move(obj.second));
          }
        }
        // TODO: make this thread-safe
        // TODO: Orvid, calling `delete` on this node causes a crash, so I'm just leaking it for now
        // for (auto &node : nodesToCleanUp){
        //   delete node;
        // }
        // nodesToCleanUp.clear();
      } else {
        // we don't have any objects, so we can just move the map
        objects = std::move(map);
      }
      return;
    }

    identifier_ref curSearchPiece = curPiece;
    identifier_ref nextSearchPiece = "";
    auto loc = curPiece.find(':');
    if (loc != identifier_ref::npos) {
      curSearchPiece = curPiece.substr(0, loc);
      nextSearchPiece = curPiece.substr(loc + 1);
    }

    auto f = children.find(curSearchPiece);
    if (f == children.end()) {
      auto n = new PapyrusNamespace();
      n->name = curSearchPiece.to_string();
      n->parent = this;
      children.emplace(n->name, n);
      f = children.find(curSearchPiece);
    }
    f->second->createNamespace(nextSearchPiece, std::move(map));
  }

  bool tryFindNamespace(const identifier_ref& curPiece, PapyrusNamespace const** ret) const {
    if (curPiece == "") {
      *ret = this;
      return true;
    }

    identifier_ref curSearchPiece = curPiece;
    identifier_ref nextSearchPiece = "";
    auto loc = curPiece.find(':');
    if (loc != identifier_ref::npos) {
      curSearchPiece = curPiece.substr(0, loc);
      nextSearchPiece = curPiece.substr(loc + 1);
    }

    auto f = children.find(curSearchPiece);
    if (f != children.end())
      return f->second->tryFindNamespace(nextSearchPiece, ret);

    return false;
  }

  bool
  tryFindType(const identifier_ref& typeName, PapyrusCompilationNode** retNode, identifier_ref* retStructName) const {
    auto loc = typeName.find(':');
    if (loc == identifier_ref::npos) {
      auto f2 = objects.find(typeName);
      if (f2 != objects.end()) {
        *retNode = f2->second;
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
      return f->second->tryFindType(subName, retNode, retStructName);

    // subName is still partially qualified, so it can't
    // be referencing a struct in this namespace.
    if (subName.find(':') != identifier_ref::npos)
      return false;

    // It is a struct reference.
    auto f2 = objects.find(baseName);
    if (f2 != objects.end()) {
      *retNode = f2->second;
      *retStructName = subName;
      return true;
    }

    return false;
  }
};
}

static PapyrusNamespace rootNamespace {};
void PapyrusCompilationContext::pushNamespaceFullContents(
    const std::string& namespaceName, caseless_unordered_identifier_ref_map<PapyrusCompilationNode*>&& map) {
  rootNamespace.createNamespace(namespaceName, std::move(map));
}

void PapyrusCompilationContext::awaitRead() {
  rootNamespace.awaitRead();
}

void PapyrusCompilationContext::doCompile(CapricaJobManager* jobManager) {
  rootNamespace.queueCompile();
  jobManager->setQueueInitialized();
  jobManager->enjoin();
}

typedef caprica::caseless_unordered_identifier_ref_map<
    caprica::caseless_unordered_identifier_ref_map<PapyrusCompilationNode*>>
    TempRenameMap;

static void renameMap(const PapyrusNamespace* child, TempRenameMap& tempRenameMap) {
  for (auto& object : child->objects) {
    auto objectName = object.second->awaitPreParse();
    auto pos = objectName.find_last_of(':');
    auto namespaceName = pos == identifier_ref::npos ? "" : objectName.substr(0, pos);
    if (tempRenameMap.count(namespaceName) == 0) {
      tempRenameMap[namespaceName] = caprica::caseless_unordered_identifier_ref_map<PapyrusCompilationNode*>();
      tempRenameMap[namespaceName].reserve(child->objects.size());
    }
    tempRenameMap[namespaceName].emplace(std::move(object.first), std::move(object.second));
  }
  for (auto& child2 : child->children)
    renameMap(child2.second, tempRenameMap);
}

void PapyrusCompilationContext::RenameImports(CapricaJobManager* jobManager) {
  // TODO: Make sure that this is actually idempotent; we call it again in main()
  if (conf::General::compileInParallel)
    jobManager->startup((uint32_t)std::thread::hardware_concurrency());

  for (auto& child : rootNamespace.children) {
    // If this is a child beginning with `!`, this is a temporary import namespace
    if (child.first[0] == '!') {
      // await parsing
      child.second->awaitPreParse();
    }
  }
  TempRenameMap tempRenameMap;
  for (auto& child : rootNamespace.children) {
    if (child.first[0] != '!')
      continue;
    renameMap(child.second, tempRenameMap);
    // this has to be done in the same import order; earlier overrides later
    for (auto& newChildMap : tempRenameMap)
      pushNamespaceFullContents(newChildMap.first.to_string(), std::move(newChildMap.second));
    tempRenameMap.clear();
  }

  // remove the children
  for (auto it = rootNamespace.children.begin(); it != rootNamespace.children.end(); ++it) {
    if ((*it).first[0] != '!')
      continue;
    rootNamespace.children.erase(it);
    it = rootNamespace.children.begin();
    if (it == rootNamespace.children.end()){
      break;
    }
  }
}

bool PapyrusCompilationContext::tryFindType(const identifier_ref& baseNamespace,
                                            const identifier_ref& typeName,
                                            PapyrusCompilationNode** retNode,
                                            identifier_ref* retStructName) {
  const PapyrusNamespace* curNamespace = nullptr;
  if (!rootNamespace.tryFindNamespace(baseNamespace, &curNamespace))
    return false;

  while (curNamespace != nullptr) {
    if (curNamespace->tryFindType(typeName, retNode, retStructName))
      return true;
    curNamespace = curNamespace->parent;
  }
  return false;
}

}}
