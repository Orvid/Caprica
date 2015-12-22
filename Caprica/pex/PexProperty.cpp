#include <pex/PexProperty.h>

namespace caprica { namespace pex {

void PexProperty::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.write<PexString>(typeName);
  wtr.write<PexString>(documentationString);
  wtr.write<PexUserFlags>(userFlags);
  uint8_t flags = 0;
  if (autoVar.valid())
    flags |= 0x07;
  else {
    if (readFunction)
      flags |= 0x01;
    if (writeFunction)
      flags |= 0x02;
  }
  wtr.write<uint8_t>(flags);
  if (autoVar.valid()) {
    wtr.write<PexString>(autoVar);
  } else {
    if (readFunction)
      readFunction->write(wtr);
    if (writeFunction)
      writeFunction->write(wtr);
  }
}

}}