#include <papyrus/PapyrusScript.h>

#include <boost/filesystem.hpp>

#include <Windows.h>
#include <lmcons.h>

#include <common/CapricaConfig.h>

namespace caprica { namespace papyrus {

pex::PexFile* PapyrusScript::buildPex() const {
  auto pex = new pex::PexFile();
  if (CapricaConfig::emitDebugInfo) {
    pex->debugInfo = new pex::PexDebugInfo();
    pex->debugInfo->modificationTime = boost::filesystem::last_write_time(sourceFileName);
  }
  pex->compilationTime = time(nullptr);
  pex->sourceFileName = sourceFileName;

  static std::string computerName = []() -> std::string {
    char compNameBuf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD compNameBufLength = sizeof(compNameBuf);
    if (!GetComputerNameA(compNameBuf, &compNameBufLength))
      CapricaError::logicalFatal("Failed to get the computer name!");
    return std::string(compNameBuf, compNameBufLength);
  }();
  pex->computerName = computerName;

  static std::string userName = []() -> std::string {
    char userNameBuf[UNLEN + 1];
    DWORD userNameBufLength = sizeof(userNameBuf);
    if (!GetUserNameA(userNameBuf, &userNameBufLength))
      CapricaError::logicalFatal("Failed to get the user name!");
    if (userNameBufLength > 0)
      userNameBufLength--;
    return std::string(userNameBuf, userNameBufLength);
  }();
  pex->userName = userName;

  for (auto o : objects)
    o->buildPex(pex);
  return pex;
}

}}
