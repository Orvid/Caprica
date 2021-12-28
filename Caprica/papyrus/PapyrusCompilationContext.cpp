#include <papyrus/PapyrusCompilationContext.h>

#include <io.h>
#include <fcntl.h>
#include <filesystem>
#include <iostream>

#include <common/CapricaConfig.h>
#include <common/allocators/AtomicChainedPool.h>

#include <papyrus/parser/PapyrusParser.h>

#include <pex/PexOptimizer.h>
#include <pex/PexReflector.h>
#include <pex/parser/PexAsmParser.h>

namespace caprica { namespace papyrus {

void PapyrusCompilationNode::awaitRead() {
  readJob.await();
}

PapyrusObject* PapyrusCompilationNode::awaitParse() {
  parseJob.await();
  return resolvedObject;
}

PapyrusObject* PapyrusCompilationNode::awaitSemantic() {
  semanticJob.await();
  return resolvedObject;
}

void PapyrusCompilationNode::queueCompile() {
  jobManager->queueJob(&writeJob);
}

void PapyrusCompilationNode::awaitWrite() {
  writeJob.await();
}

allocators::AtomicChainedPool readAllocator{ 1024 * 1024 * 4 };
void PapyrusCompilationNode::FileReadJob::run() {
  if (parent->type == NodeType::PapyrusCompile || parent->type == NodeType::PasCompile || parent->type == NodeType::PexDissassembly) {
    if (!conf::General::quietCompile)
      std::cout << "Compiling " << parent->reportedName << std::endl;
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
    std::ifstream inFile{ parent->sourceFilePath, std::ifstream::binary };
    inFile.exceptions(std::ifstream::badbit | std::ifstream::failbit);
    if (parent->filesize != 0)
      inFile.read((char*)str.data(), parent->filesize);
    // Just because the filesize was one thing when
    // we iterated the directory doesn't mean it's
    // not gotten bigger since then.
    inFile.peek();
    if (!inFile.eof()) {
      std::stringstream strStream{ };
      strStream.exceptions(std::ifstream::badbit | std::ifstream::failbit);
      strStream << inFile.rdbuf();
      str += strStream.str();
    }
    str += '\0';
    parent->ownedReadFileData = std::move(str);
    parent->readFileData = parent->ownedReadFileData;
  }
}

void PapyrusCompilationNode::FileParseJob::run() {
  parent->readJob.await();
  bool isPexFile = false;
  auto ext = FSUtils::extensionAsRef(parent->sourceFilePath);
  if (pathEq(ext, ".psc")) {
    auto parser = new parser::PapyrusParser(parent->reportingContext, parent->sourceFilePath, parent->readFileData);
    parent->loadedScript = parser->parseScript();
    parent->reportingContext.exitIfErrors();
    delete parser;
  } else if (pathEq(ext, ".pex")) {
    pex::PexReader rdr(parent->sourceFilePath);
    auto alloc = new allocators::ChainedPool(1024 * 4);
    parent->pexFile = pex::PexFile::read(alloc, rdr);
    isPexFile = true;
    if (parent->type == NodeType::PexDissassembly)
      return;
  } else if (pathEq(ext, ".pas")) {
    auto parser = new pex::parser::PexAsmParser(parent->reportingContext, parent->sourceFilePath);
    parent->pexFile = parser->parseFile();
    parent->reportingContext.exitIfErrors();
    delete parser;
    isPexFile = true;
    if (parent->type == NodeType::PasCompile)
      return;
  } else {
    CapricaReportingContext::logicalFatal("Unable to determine the type of file to load '%s' as.", parent->reportedName.c_str());
  }

  if (parent->pexFile) {
    parent->loadedScript = pex::PexReflector::reflectScript(parent->pexFile);
    parent->reportingContext.exitIfErrors();
    delete parent->pexFile->alloc;
    parent->pexFile = nullptr;
  }

  assert(parent->loadedScript != nullptr);

  for (auto o : parent->loadedScript->objects)
    o->compilationNode = parent;
  parent->resolutionContext = new PapyrusResolutionContext(parent->reportingContext);
  parent->resolutionContext->allocator = parent->loadedScript->allocator;
  parent->resolutionContext->isPexResolution = isPexFile;
  parent->loadedScript->preSemantic(parent->resolutionContext);
  parent->reportingContext.exitIfErrors();
  
  if (parent->loadedScript->objects.size() != 1)
    CapricaReportingContext::logicalFatal("The script had either no objects or more than one!");
  parent->resolvedObject = parent->loadedScript->objects.front();
}

void PapyrusCompilationNode::FileSemanticJob::run() {
  parent->parseJob.await();
  parent->loadedScript->semantic(parent->resolutionContext);
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
          std::ofstream asmStrm(parent->outputDirectory + "\\" + std::string(parent->baseName) + ".pas", std::ofstream::binary);
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
      std::ofstream asmStrm(parent->outputDirectory + "\\" + std::string(parent->baseName) + ".pas", std::ofstream::binary);
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
    case NodeType::Unknown:
    case NodeType::PasReflection:
    case NodeType::PexReflection:
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
        std::ofstream destFile{ parent->outputDirectory + "\\" + baseFileName + ".pex", std::ifstream::binary };
        destFile.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        parent->pexWriter->applyToBuffers([&](const char* data, size_t size) {
          destFile.write(data, size);
        });
      }
      delete parent->pexWriter;
      parent->pexWriter = nullptr;
      return;
    }
    case NodeType::Unknown:
    case NodeType::PexDissassembly:
    case NodeType::PasReflection:
    case NodeType::PexReflection:
      break;
  }
  CapricaReportingContext::logicalFatal("You shouldn't be trying to compile this!");
}

namespace {

struct PapyrusNamespace final
{
  std::string name{ "" };
  PapyrusNamespace* parent{ nullptr };
  caseless_unordered_identifier_ref_map<PapyrusNamespace*> children{ };
  // Key is unqualified name, value is full path to file.
  caseless_unordered_identifier_ref_map<PapyrusCompilationNode*> objects{ };

  void awaitRead() {
    for (auto o : objects)
      o.second->awaitRead();
    for (auto c : children)
      c.second->awaitRead();
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

  void createNamespace(const identifier_ref& curPiece, caseless_unordered_identifier_ref_map<PapyrusCompilationNode*>&& map) {
    if (curPiece == "") {
      objects = std::move(map);
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

  bool tryFindType(const identifier_ref& typeName, PapyrusCompilationNode** retNode, identifier_ref* retStructName) const {
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

static PapyrusNamespace rootNamespace{ };
void PapyrusCompilationContext::pushNamespaceFullContents(const std::string& namespaceName, caseless_unordered_identifier_ref_map<PapyrusCompilationNode*>&& map) {
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

bool PapyrusCompilationContext::tryFindType(const identifier_ref& baseNamespace, const identifier_ref& typeName, PapyrusCompilationNode** retNode, identifier_ref* retStructName) {
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
