#include <pex/PexStructMember.h>

namespace caprica { namespace pex {

void PexStructMember::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.write<PexString>(typeName);
  wtr.write<PexUserFlags>(userFlags);
  wtr.write<PexValue>(defaultValue);
  wtr.write<uint8_t>(isConst ? 0x01 : 0x00);
  wtr.write<PexString>(documentationString);
}

}}