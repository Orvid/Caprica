#include <papyrus/PapyrusScriptLoader.h>

#include <cassert>
#include <iostream>
#include <memory>

#include <common/CapricaConfig.h>
#include <common/CaselessStringComparer.h>
#include <common/FSUtils.h>

#include <papyrus/parser/PapyrusParser.h>

#include <pex/PexOptimizer.h>
#include <pex/PexReflector.h>
#include <pex/parser/PexAsmParser.h>

namespace caprica { namespace papyrus {

// This is safe because it will only ever contain scripts referencing items in this map, and this map
// will never contain a fully-resolved script.
static thread_local caseless_unordered_path_map<std::string, std::unique_ptr<PapyrusScript>> loadedScripts{ };
PapyrusScript* PapyrusScriptLoader::loadScript(const std::string& reportedName,
                                               const std::string& fullPath,
                                               const std::string& baseOutputDirectory,
                                               LoadType loadType) {
  if (loadType == LoadType::Reference) {
    auto f = loadedScripts.find(fullPath);
    if (f != loadedScripts.end())
      return f->second.get();
  }

  CapricaReportingContext repCtx{ reportedName };
  PapyrusScript* loadedScript = nullptr;
  pex::PexFile* pexFile = nullptr;
  bool isPexFile = false;

  auto ext = FSUtils::extensionAsRef(fullPath);
  if (pathEq(ext, ".psc")) {
    auto parser = new parser::PapyrusParser(repCtx, fullPath);
    loadedScript = parser->parseScript();
    repCtx.exitIfErrors();
    delete parser;
  } else if (pathEq(ext, ".pex")) {
    pex::PexReader rdr(fullPath);
    pexFile = pex::PexFile::read(rdr);
    isPexFile = true;

    if (loadType == LoadType::Compile) {
      auto baseName = FSUtils::basenameAsRef(fullPath).to_string();
      std::ofstream asmStrm(baseOutputDirectory + "\\" + baseName + ".pas", std::ofstream::binary);
      asmStrm.exceptions(std::ifstream::badbit | std::ifstream::failbit);
      caprica::pex::PexAsmWriter asmWtr(asmStrm);
      pexFile->writeAsm(asmWtr);
      delete pexFile;
      return nullptr;
    }
  } else if (pathEq(ext, ".pas")) {
    auto parser = new pex::parser::PexAsmParser(repCtx, fullPath);
    pexFile = parser->parseFile();
    repCtx.exitIfErrors();
    delete parser;
    isPexFile = true;

    if (loadType == LoadType::Compile) {
      if (conf::CodeGeneration::enableOptimizations)
        pex::PexOptimizer::optimize(pexFile);

      pex::PexWriter wtr{ };
      pexFile->write(wtr);
      auto baseName = FSUtils::basenameAsRef(fullPath).to_string();
      wtr.writeToFile(baseOutputDirectory + "\\" + baseName + ".pex");
      delete pexFile;
      return nullptr;
    }
  } else {
    CapricaReportingContext::logicalFatal("Unable to determine the type of file to load '%s' as.", reportedName.c_str());
  }

  if (pexFile) {
    assert(loadType == LoadType::Reference);
    loadedScript = pex::PexReflector::reflectScript(pexFile);
    repCtx.exitIfErrors();
    delete pexFile;
    pexFile = nullptr;
  }

  assert(loadedScript != nullptr);

  auto ctx = new PapyrusResolutionContext(repCtx);
  ctx->resolvingReferenceScript = loadType == LoadType::Reference;
  ctx->isPexResolution = isPexFile;
  loadedScript->preSemantic(ctx);
  repCtx.exitIfErrors();

  if (loadType == LoadType::Reference) {
    auto ptr = std::unique_ptr<PapyrusScript>(loadedScript);
    loadedScripts.emplace(fullPath, std::move(ptr));
    loadedScript = loadedScripts[fullPath].get();
  }

  loadedScript->semantic(ctx);
  repCtx.exitIfErrors();

  if (loadType != LoadType::Reference && loadType != LoadType::CheckOnly) {
    loadedScript->semantic2(ctx);
    repCtx.exitIfErrors();
  }

  delete ctx;
  ctx = nullptr;

  if (loadType == LoadType::Reference || loadType == LoadType::CheckOnly)
    return loadedScript;

  assert(loadType == LoadType::Compile);
  assert(pathEq(ext, ".psc"));

  pexFile = loadedScript->buildPex(repCtx);
  repCtx.exitIfErrors();
  delete loadedScript;
  loadedScript = nullptr;

  if (conf::CodeGeneration::enableOptimizations)
    pex::PexOptimizer::optimize(pexFile);

  pex::PexWriter wtr{ };
  pexFile->write(wtr);
  auto baseName = FSUtils::basenameAsRef(fullPath).to_string();
  wtr.writeToFile(baseOutputDirectory + "\\" + baseName + ".pex");

  if (conf::Debug::dumpPexAsm) {
    std::ofstream asmStrm(baseOutputDirectory + "\\" + baseName + ".pas", std::ofstream::binary);
    asmStrm.exceptions(std::ifstream::badbit | std::ifstream::failbit);
    pex::PexAsmWriter asmWtr(asmStrm);
    pexFile->writeAsm(asmWtr);
  }

  delete pexFile;
  return nullptr;
}

}}
