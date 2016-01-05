#include <pex/PexStruct.h>

#include <pex/PexFile.h>

namespace caprica { namespace pex {

void PexStruct::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.boundWrite<uint16_t>(members.size());
  for (auto m : members)
    m->write(wtr);
}

void PexStruct::writeAsm(const PexFile* file, PexAsmWriter& wtr) const {
  wtr.writeln(".struct %s", file->getStringValue(name).c_str());
  wtr.ident++;
  for (auto m : members)
    m->writeAsm(file, wtr);
  wtr.ident--;
  wtr.writeln(".endStruct");
}

}}
