#include <pex/PexVariable.h>

#include <pex/PexFile.h>

namespace caprica { namespace pex {

void PexVariable::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.write<PexString>(typeName);
  wtr.write<PexUserFlags>(userFlags);
  wtr.write<PexValue>(defaultValue);
  wtr.write<uint8_t>(isConst ? 0x01 : 0x00);
}

void PexVariable::writeAsm(const PexFile* file, PexAsmWriter& wtr) const {
  wtr.writeln(".variable %s %s", file->getStringValue(name).c_str(), file->getStringValue(typeName).c_str());
  wtr.ident++;
  wtr.writeKV<PexUserFlags>("userFlags", userFlags);
  wtr.write(".initialValue ");
  defaultValue.writeAsm(file, wtr);
  wtr.writeln();
  wtr.ident--;
  wtr.writeln(".endVariable");
}

}}
