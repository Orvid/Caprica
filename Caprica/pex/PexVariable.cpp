#include <pex/PexVariable.h>

namespace caprica { namespace pex {

void PexVariable::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.write<PexString>(typeName);
  wtr.write<PexUserFlags>(userFlags);
  wtr.write<PexValue>(defaultValue);
  wtr.write<uint8_t>(isConst ? 0x01 : 0x00);
}

}}