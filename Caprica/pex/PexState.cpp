#include <pex/PexState.h>

#include <pex/PexDebugFunctionInfo.h>
#include <pex/PexFile.h>
#include <pex/PexObject.h>

namespace caprica { namespace pex {

void PexState::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.boundWrite<uint16_t>(functions.size());
  for (auto f : functions)
    f->write(wtr);
}

void PexState::writeAsm(const PexFile* file, const PexObject* obj, PexAsmWriter& wtr) const {
  wtr.write(".state");
  if (file->getStringValue(name) != "")
    wtr.write(" %s", file->getStringValue(name).c_str());
  wtr.writeln();
  wtr.ident++;

  for (auto f : functions)
    f->writeAsm(file, obj, this, PexDebugFunctionType::Normal, "", wtr);

  wtr.ident--;
  wtr.writeln(".endState");
}

}}
