#include <pex/PexStructMember.h>

#include <pex/PexFile.h>

namespace caprica { namespace pex {

void PexStructMember::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.write<PexString>(typeName);
  wtr.write<PexUserFlags>(userFlags);
  wtr.write<PexValue>(defaultValue);
  wtr.write<uint8_t>(isConst ? 0x01 : 0x00);
  wtr.write<PexString>(documentationString);
}

void PexStructMember::writeAsm(const PexFile* file, PexAsmWriter& wtr) const {
  wtr.writeln(".member %s %s", file->getStringValue(name).c_str(), file->getStringValue(typeName).c_str());
  wtr.ident++;
  wtr.writeln(".constFlag %i", isConst ? 1 : 0);
  wtr.writeKV<PexUserFlags>("userFlags", userFlags);
  wtr.writeKV<std::string>("docString", file->getStringValue(documentationString));
  wtr.write(".initialValue ");
  defaultValue.writeAsm(file, wtr);
  wtr.writeln();
  wtr.ident--;
  wtr.writeln(".endMember");
}

}}
