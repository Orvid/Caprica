#include <pex/PexStructInfo.h>

#include <pex/PexFile.h>

namespace caprica { namespace pex {

void PexStructInfo::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.boundWrite<uint16_t>(members.size());
  for (auto m : members)
    m->write(wtr);
}

void PexStructInfo::writeAsm(const PexFile* file, PexAsmWriter& wtr) const {
  wtr.writeln(".struct %s", file->getStringValue(name).c_str());
  wtr.ident++;
  for (auto m : members)
    m->writeAsm(file, wtr);
  wtr.ident--;
  wtr.writeln(".endStruct");
}

}}
