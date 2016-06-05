#pragma once

#include <string>

#include <common/CapricaJobManager.h>
#include <common/CaselessStringComparer.h>
#include <common/FSUtils.h>

#include <papyrus/PapyrusScript.h>

namespace caprica { namespace papyrus {

struct PapyrusCompilationNode final
{
  enum class NodeType
  {
    Unknown,

    PapyrusCompile,

    PasCompile,
    PexDissassembly,

    PasReflection,
    PexReflection,
  };

  PapyrusCompilationNode() = delete;
  PapyrusCompilationNode(CapricaJobManager* mgr, NodeType compileType, std::string&& sourcePath,
                         std::string&& baseOutputDir, std::string&& absolutePath,
                         time_t lastMod, size_t fileSize)
    : reportedName(std::move(sourcePath)),
    outputDirectory(std::move(baseOutputDir)),
    sourceFilePath(std::move(absolutePath)),
    lastModTime(lastMod),
    filesize(fileSize),
    reportingContext(reportedName),
    jobManager(mgr),
    type(compileType) {
    jobManager->queueJob(&readJob);
  }

  ~PapyrusCompilationNode() {
    if (loadedScript)
      delete loadedScript;
    if (pexFile)
      delete pexFile;
    if (resolvedObject)
      delete resolvedObject;
    if (resolutionContext)
      delete resolutionContext;
  }

  void queueCompile();
  void awaitCompile();
  PapyrusObject* awaitParse();
  PapyrusObject* awaitSemantic();

private:
  struct BaseJob : public CapricaJob {
    BaseJob(PapyrusCompilationNode* par) : parent(par) { }
  protected:
    PapyrusCompilationNode* parent;
  };

  NodeType type;
  size_t filesize;
  time_t lastModTime;
  std::string reportedName;
  std::string outputDirectory;
  std::string sourceFilePath;
  std::string readFileData{ };
  PapyrusScript* loadedScript{ nullptr };
  pex::PexFile* pexFile{ nullptr };
  PapyrusObject* resolvedObject{ nullptr };
  CapricaReportingContext reportingContext;
  PapyrusResolutionContext* resolutionContext{ nullptr };
  CapricaJobManager* jobManager;

  struct FileReadJob final : public BaseJob {
    using BaseJob::BaseJob;
    virtual void run() override;
  } readJob{ this };
  struct FileParseJob final : public BaseJob {
    using BaseJob::BaseJob;
    virtual void run() override;
  } parseJob{ this };
  struct FileSemanticJob final : public BaseJob
  {
    using BaseJob::BaseJob;
    virtual void run() override;
  } semanticJob{ this };
  struct FileCompileJob final : public BaseJob
  {
    using BaseJob::BaseJob;
    virtual void run() override;
  } compileJob{ this };
};

struct PapyrusCompilationContext final
{
  static void doCompile();
  static void pushNamespaceFullContents(const std::string& namespaceName, caseless_unordered_identifier_map<PapyrusCompilationNode*>&& map);
  static bool tryFindType(boost::string_ref baseNamespace, const std::string& typeName, PapyrusCompilationNode** retNode, std::string* retStructName);
};

}}
