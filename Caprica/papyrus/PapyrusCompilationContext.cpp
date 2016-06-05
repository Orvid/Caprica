#include <papyrus/PapyrusCompilationContext.h>

#include <io.h>
#include <fcntl.h>

#include <iostream>

#include <common/CapricaConfig.h>

#include <papyrus/parser/PapyrusParser.h>

#include <pex/PexOptimizer.h>
#include <pex/PexReflector.h>
#include <pex/parser/PexAsmParser.h>

namespace caprica { namespace papyrus {

PapyrusObject* PapyrusCompilationNode::awaitParse() {
  parseJob.await();
  return resolvedObject;
}

PapyrusObject* PapyrusCompilationNode::awaitSemantic() {
  semanticJob.await();
  return resolvedObject;
}

void PapyrusCompilationNode::queueCompile() {
  jobManager->queueJob(&compileJob);
}

void PapyrusCompilationNode::awaitCompile() {
  compileJob.await();
}

void PapyrusCompilationNode::FileReadJob::run() {
  if (parent->type == NodeType::PapyrusCompile || parent->type == NodeType::PasCompile || parent->type == NodeType::PexDissassembly) {
    if (!conf::General::quietCompile)
      std::cout << "Compiling " << parent->reportedName << std::endl;
  }
  std::string str;
  str.resize(parent->filesize);
  if (parent->filesize < std::numeric_limits<uint32_t>::max()) {
    auto fd = _open(parent->sourceFilePath.c_str(), _O_BINARY | _O_RDONLY | _O_SEQUENTIAL);
    if (fd != -1) {
      auto len = _read(fd, (void*)str.data(), (uint32_t)parent->filesize);
      if (len != parent->filesize)
        str.resize(len);
      if (_eof(fd) == 1) {
        _close(fd);
        goto DoMove;
      }
      _close(fd);
    }
  }
  {
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
  }
DoMove:
  parent->readFileData = std::move(str);
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
    parent->pexFile = pex::PexFile::read(rdr);
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
    delete parent->pexFile;
    parent->pexFile = nullptr;
  }

  assert(parent->loadedScript != nullptr);

  for (auto o : parent->loadedScript->objects)
    o->compilationNode = parent;
  parent->resolutionContext = new PapyrusResolutionContext(parent->reportingContext);
  parent->resolutionContext->isPexResolution = isPexFile;
  parent->loadedScript->preSemantic(parent->resolutionContext);
  parent->reportingContext.exitIfErrors();
  
  if (parent->loadedScript->objects.size() != 1)
    CapricaReportingContext::logicalFatal("The script had either no objects or more than one!");
  parent->resolvedObject = parent->loadedScript->objects[0];
}

void PapyrusCompilationNode::FileSemanticJob::run() {
  parent->parseJob.await();
  parent->loadedScript->semantic(parent->resolutionContext);
  parent->reportingContext.exitIfErrors();
}

void PapyrusCompilationNode::FileCompileJob::run() {
  parent->semanticJob.await();
  switch (parent->type) {
    case NodeType::PapyrusCompile: {
      parent->loadedScript->semantic2(parent->resolutionContext);
      parent->reportingContext.exitIfErrors();

      parent->pexFile = parent->loadedScript->buildPex(parent->reportingContext);
      parent->reportingContext.exitIfErrors();

      if (conf::CodeGeneration::enableOptimizations)
        pex::PexOptimizer::optimize(parent->pexFile);

      pex::PexWriter wtr{ };
      parent->pexFile->write(wtr);
      auto baseName = FSUtils::basenameAsRef(parent->sourceFilePath).to_string();
      wtr.writeToFile(parent->outputDirectory + "\\" + baseName + ".pex");

      if (conf::Debug::dumpPexAsm) {
        std::ofstream asmStrm(parent->outputDirectory + "\\" + baseName + ".pas", std::ofstream::binary);
        asmStrm.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        pex::PexAsmWriter asmWtr(asmStrm);
        parent->pexFile->writeAsm(asmWtr);
      }

      delete parent->pexFile;
      parent->pexFile = nullptr;
      return;
    }
    case NodeType::PexDissassembly: {
      auto baseName = FSUtils::basenameAsRef(parent->sourceFilePath).to_string();
      std::ofstream asmStrm(parent->outputDirectory + "\\" + baseName + ".pas", std::ofstream::binary);
      asmStrm.exceptions(std::ifstream::badbit | std::ifstream::failbit);
      caprica::pex::PexAsmWriter asmWtr(asmStrm);
      parent->pexFile->writeAsm(asmWtr);
      delete parent->pexFile;
      parent->pexFile = nullptr;
      return;
    }
    case NodeType::PasCompile: {
      if (conf::CodeGeneration::enableOptimizations)
        pex::PexOptimizer::optimize(parent->pexFile);

      pex::PexWriter wtr{ };
      parent->pexFile->write(wtr);
      auto baseName = FSUtils::basenameAsRef(parent->sourceFilePath).to_string();
      wtr.writeToFile(parent->outputDirectory + "\\" + baseName + ".pex");
      delete parent->pexFile;
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


namespace {

struct PapyrusNamespace final
{
  std::string name{ "" };
  PapyrusNamespace* parent{ nullptr };
  caseless_unordered_identifier_map<PapyrusNamespace*> children{ };
  // Key is unqualified name, value is full path to file.
  caseless_unordered_identifier_map<PapyrusCompilationNode*> objects{ };

  void queueCompile() {
    for (auto o : objects)
      o.second->queueCompile();
    for (auto c : children)
      c.second->queueCompile();
  }

  void awaitCompile() {
    for (auto o : objects)
      o.second->awaitCompile();
    for (auto c : children)
      c.second->awaitCompile();
  }

  void createNamespace(boost::string_ref curPiece, caseless_unordered_identifier_map<PapyrusCompilationNode*>&& map) {
    if (curPiece == "") {
      objects = std::move(map);
      return;
    }

    boost::string_ref curSearchPiece = curPiece;
    boost::string_ref nextSearchPiece = "";
    auto loc = curPiece.find(':');
    if (loc != boost::string_ref::npos) {
      curSearchPiece = curPiece.substr(0, loc);
      nextSearchPiece = curPiece.substr(loc + 1);
    }

    auto curSearchStr = curSearchPiece.to_string();
    auto f = children.find(curSearchStr);
    if (f == children.end()) {
      auto n = new PapyrusNamespace();
      n->name = curSearchStr;
      n->parent = this;
      children.insert({ curSearchStr, n });
      f = children.find(curSearchStr);
    }
    f->second->createNamespace(nextSearchPiece, std::move(map));
  }

  bool tryFindNamespace(boost::string_ref curPiece, PapyrusNamespace const** ret) const {
    if (curPiece == "") {
      *ret = this;
      return true;
    }

    boost::string_ref curSearchPiece = curPiece;
    boost::string_ref nextSearchPiece = "";
    auto loc = curPiece.find(':');
    if (loc != boost::string_ref::npos) {
      curSearchPiece = curPiece.substr(0, loc);
      nextSearchPiece = curPiece.substr(loc + 1);
    }

    auto f = children.find(curSearchPiece.to_string());
    if (f != children.end())
      return f->second->tryFindNamespace(nextSearchPiece, ret);

    return false;
  }

  bool tryFindType(boost::string_ref typeName, PapyrusCompilationNode** retNode, std::string* retStructName) const {
    auto loc = typeName.find(':');
    if (loc == boost::string_ref::npos) {
      auto tnStr = typeName.to_string();
      auto f2 = objects.find(tnStr);
      if (f2 != objects.end()) {
        *retNode = f2->second;
        return true;
      }
      return false;
    }

    // It's a partially qualified type name, or else is referencing
    // a struct.
    auto baseName = typeName.substr(0, loc).to_string();
    auto subName = typeName.substr(loc + 1);

    // It's a partially qualified name.
    auto f = children.find(baseName);
    if (f != children.end())
      return f->second->tryFindType(subName, retNode, retStructName);

    // subName is still partially qualified, so it can't
    // be referencing a struct in this namespace.
    if (subName.find(':') != boost::string_ref::npos)
      return false;

    // It is a struct reference.
    auto f2 = objects.find(baseName);
    if (f2 != objects.end()) {
      *retNode = f2->second;
      *retStructName = subName.to_string();
      return true;
    }

    return false;
  }
};
}

static PapyrusNamespace rootNamespace{ };
void PapyrusCompilationContext::pushNamespaceFullContents(const std::string& namespaceName, caseless_unordered_identifier_map<PapyrusCompilationNode*>&& map) {
  rootNamespace.createNamespace(namespaceName, std::move(map));
}

void PapyrusCompilationContext::doCompile() {
  rootNamespace.queueCompile();
  rootNamespace.awaitCompile();
}

bool PapyrusCompilationContext::tryFindType(const std::string& baseNamespace, const std::string& typeName, PapyrusCompilationNode** retNode, std::string* retStructName) {
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
