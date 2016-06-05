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

  boost::string_ref baseName;

  PapyrusCompilationNode() = delete;
  PapyrusCompilationNode(CapricaJobManager* mgr, NodeType compileType, std::string&& sourcePath,
                         std::string&& baseOutputDir, std::string&& absolutePath,
                         time_t lastMod, size_t fileSize) :
    reportedName(std::move(sourcePath)),
    outputDirectory(std::move(baseOutputDir)),
    sourceFilePath(std::move(absolutePath)),
    lastModTime(lastMod),
    filesize(fileSize),
    reportingContext(reportedName),
    jobManager(mgr),
    type(compileType) {
    baseName = FSUtils::basenameAsRef(sourceFilePath);
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

  PapyrusObject* awaitParse();
  PapyrusObject* awaitSemantic();
  void queueCompile();
  void awaitWrite();

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
  boost::string_ref readFileData{ };
  std::string ownedReadFileData{ };
  std::string dataToWrite{ };
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
  struct FileWriteJob final : public BaseJob
  {
    using BaseJob::BaseJob;
    virtual void run() override;
  } writeJob{ this };
};

struct PapyrusCompilationContext final
{
  static void doCompile();
  static void pushNamespaceFullContents(const std::string& namespaceName, caseless_unordered_identifier_ref_map<PapyrusCompilationNode*>&& map);
  static bool tryFindType(boost::string_ref baseNamespace, const std::string& typeName, PapyrusCompilationNode** retNode, boost::string_ref* retStructName);
};

}}
