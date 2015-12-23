#include <pex/PexFunctionParameter.h>

namespace caprica { namespace pex {

void PexFunctionParameter::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.write<PexString>(type);
}

}}