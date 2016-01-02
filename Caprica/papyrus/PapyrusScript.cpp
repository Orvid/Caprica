#include <papyrus/PapyrusScript.h>

#include <boost/filesystem.hpp>

#include <Windows.h>
#include <lmcons.h>

#include <CapricaConfig.h>

namespace caprica { namespace papyrus {

pex::PexFile* PapyrusScript::buildPex() const {
  auto pex = new pex::PexFile();
  if (CapricaConfig::emitDebugInfo) {
    pex->debugInfo = new pex::PexDebugInfo();
    pex->debugInfo->modificationTime = boost::filesystem::last_write_time(sourceFileName);
  }
  pex->compilationTime = time(nullptr);
  pex->sourceFileName = sourceFileName;

  char compNameBuf[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD compNameBufLength = sizeof(compNameBuf);
  if (!GetComputerNameA(compNameBuf, &compNameBufLength))
    CapricaError::logicalFatal("Failed to get the computer name!");
  pex->computerName = std::string(compNameBuf, compNameBufLength);

  char userNameBuf[UNLEN + 1];
  DWORD userNameBufLength = sizeof(userNameBuf);
  if (!GetUserNameA(userNameBuf, &userNameBufLength))
    CapricaError::logicalFatal("Failed to get the user name!");
  if (userNameBufLength > 0)
    userNameBufLength--;
  pex->userName = std::string(userNameBuf, userNameBufLength);

  for (auto o : objects)
    o->buildPex(pex);
  return pex;
}

}}
