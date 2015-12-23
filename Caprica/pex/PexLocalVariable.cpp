#include <pex/PexLocalVariable.h>

#include <pex/PexWriter.h>

namespace caprica { namespace pex {

void PexLocalVariable::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.write<PexString>(type);
}

}}