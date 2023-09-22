#include <pex/PexFunctionParameter.h>

#include <pex/PexFile.h>

namespace caprica { namespace pex {

PexFunctionParameter* PexFunctionParameter::read(allocators::ChainedPool* alloc, PexReader& rdr) {
  auto param = alloc->make<PexFunctionParameter>();
  param->name = rdr.read<PexString>();
  param->type = rdr.read<PexString>();
  return param;
}

void PexFunctionParameter::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.write<PexString>(type);
}

void PexFunctionParameter::writeAsm(const PexFile* file, PexAsmWriter& wtr) const {
  wtr.writeln(".param %s %s",
              file->getStringValue(name).to_string().c_str(),
              file->getStringValue(type).to_string().c_str());
}

}}
