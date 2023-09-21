#include <pex/PexVariable.h>

#include <pex/PexFile.h>

namespace caprica { namespace pex {

PexVariable* PexVariable::read(allocators::ChainedPool* alloc, PexReader& rdr) {
  auto var = alloc->make<PexVariable>();
  var->name = rdr.read<PexString>();
  var->typeName = rdr.read<PexString>();
  var->userFlags = rdr.read<PexUserFlags>();
  var->defaultValue = rdr.read<PexValue>();
  var->isConst = rdr.read<uint8_t>() != 0;
  return var;
}

void PexVariable::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.write<PexString>(typeName);
  wtr.write<PexUserFlags>(userFlags);
  wtr.write<PexValue>(defaultValue);
  wtr.write<uint8_t>(isConst ? 0x01 : 0x00);
}

void PexVariable::writeAsm(const PexFile* file, PexAsmWriter& wtr) const {
  wtr.write(".variable %s %s", file->getStringValue(name).to_string().c_str(), file->getStringValue(typeName).to_string().c_str());
  if (isConst)
    wtr.write(" const");
  wtr.writeln();
  wtr.ident++;
  wtr.writeKV<PexUserFlags>("userFlags", userFlags);
  wtr.write(".initialValue ");
  defaultValue.writeAsm(file, wtr);
  wtr.writeln();
  wtr.ident--;
  wtr.writeln(".endVariable");
}

}}
