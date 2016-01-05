#include <pex/PexLocalVariable.h>

#include <pex/PexFile.h>
#include <pex/PexWriter.h>

namespace caprica { namespace pex {

void PexLocalVariable::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.write<PexString>(type);
}

void PexLocalVariable::writeAsm(const PexFile* file, PexAsmWriter& wtr) const {
  wtr.writeln(".local %s %s", file->getStringValue(name).c_str(), file->getStringValue(type).c_str());
}

}}
