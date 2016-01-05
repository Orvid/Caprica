#include <pex/PexFunctionParameter.h>

#include <pex/PexFile.h>

namespace caprica { namespace pex {

void PexFunctionParameter::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.write<PexString>(type);
}

void PexFunctionParameter::writeAsm(const PexFile* file, PexAsmWriter& wtr) const {
  wtr.writeln(".param %s %s", file->getStringValue(name).c_str(), file->getStringValue(type).c_str());
}

}}
