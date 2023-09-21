#include <pex/PexGuard.h>

#include <pex/PexDebugFunctionInfo.h>
#include <pex/PexFile.h>

namespace caprica { namespace pex {

PexGuard* PexGuard::read(allocators::ChainedPool* alloc, PexReader& rdr) {
  auto prop = alloc->make<PexGuard>();
  prop->name = rdr.read<PexString>();
  return prop;
}

void PexGuard::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
}

void PexGuard::writeAsm(const PexFile* file, PexAsmWriter& wtr) const {
  wtr.writeln(".guard %s", file->getStringValue(name).to_string().c_str());
}

}}
