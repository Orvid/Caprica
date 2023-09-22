#include <pex/PexLocalVariable.h>

#include <pex/PexFile.h>
#include <pex/PexReader.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

PexLocalVariable* PexLocalVariable::read(allocators::ChainedPool* alloc, PexReader& rdr) {
  auto loc = alloc->make<PexLocalVariable>();
  loc->name = rdr.read<PexString>();
  loc->type = rdr.read<PexString>();
  return loc;
}

void PexLocalVariable::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.write<PexString>(type);
}

void PexLocalVariable::writeAsm(const PexFile* file, PexAsmWriter& wtr) const {
  wtr.writeln(".local %s %s",
              file->getStringValue(name).to_string().c_str(),
              file->getStringValue(type).to_string().c_str());
}

}}
