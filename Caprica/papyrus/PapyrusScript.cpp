#include <papyrus/PapyrusScript.h>

#include <Windows.h>
#include <lmcons.h>

#include <common/CapricaConfig.h>
#include <common/EngineLimits.h>

namespace caprica { namespace papyrus {

pex::PexFile* PapyrusScript::buildPex(CapricaReportingContext& repCtx) const {
  auto pex = new pex::PexFile();
  if (conf::CodeGeneration::emitDebugInfo) {
    pex->debugInfo = new pex::PexDebugInfo();
    pex->debugInfo->modificationTime = lastModificationTime;
  }
  pex->compilationTime = time(nullptr);
  pex->sourceFileName = sourceFileName;

  static std::string computerName = []() -> std::string {
    char compNameBuf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD compNameBufLength = sizeof(compNameBuf);
    if (!GetComputerNameA(compNameBuf, &compNameBufLength))
      CapricaReportingContext::logicalFatal("Failed to get the computer name!");
    return std::string(compNameBuf, compNameBufLength);
  }();
  pex->computerName = computerName;

  static std::string userName = []() -> std::string {
    char userNameBuf[UNLEN + 1];
    DWORD userNameBufLength = sizeof(userNameBuf);
    if (!GetUserNameA(userNameBuf, &userNameBufLength))
      CapricaReportingContext::logicalFatal("Failed to get the user name!");
    if (userNameBufLength > 0)
      userNameBufLength--;
    return std::string(userNameBuf, userNameBufLength);
  }();
  pex->userName = userName;

  for (auto o : objects)
    o->buildPex(repCtx, pex);

  if (objects.size())
    EngineLimits::checkLimit(repCtx, objects.front()->location, EngineLimits::Type::PexFile_UserFlagCount, pex->getUserFlagCount());
  return pex;
}

}}
