#include <pex/PexStructMember.h>

#include <pex/PexFile.h>

namespace caprica { namespace pex {

PexStructMember* PexStructMember::read(allocators::ChainedPool* alloc, PexReader& rdr) {
  auto mem = alloc->make<PexStructMember>();
  mem->name = rdr.read<PexString>();
  mem->typeName = rdr.read<PexString>();
  mem->userFlags = rdr.read<PexUserFlags>();
  mem->defaultValue = rdr.read<PexValue>();
  mem->isConst = rdr.read<uint8_t>() != 0;
  mem->documentationString = rdr.read<PexString>();
  return mem;
}

void PexStructMember::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.write<PexString>(typeName);
  wtr.write<PexUserFlags>(userFlags);
  wtr.write<PexValue>(defaultValue);
  wtr.write<uint8_t>(isConst ? 0x01 : 0x00);
  wtr.write<PexString>(documentationString);
}

void PexStructMember::writeAsm(const PexFile* file, PexAsmWriter& wtr) const {
  wtr.write(".variable %s %s", file->getStringValue(name).to_string().c_str(), file->getStringValue(typeName).to_string().c_str());
  if (isConst)
    wtr.write(" const");
  wtr.writeln();
  wtr.ident++;
  wtr.writeKV<PexUserFlags>("userFlags", userFlags);
  wtr.write(".initialValue ");
  defaultValue.writeAsm(file, wtr);
  wtr.writeln();
  wtr.writeKV<std::string>("docString", file->getStringValue(documentationString).to_string());
  wtr.ident--;
  wtr.writeln(".endVariable");
}

}}
