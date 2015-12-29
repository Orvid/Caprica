#include <pex/PexProperty.h>

namespace caprica { namespace pex {

void PexProperty::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.write<PexString>(typeName);
  wtr.write<PexString>(documentationString);
  wtr.write<PexUserFlags>(userFlags);
  uint8_t flags = 0;
  if (isReadable)
    flags |= 0x01;
  if (isWritable)
    flags |= 0x02;
  if (isAuto)
    flags |= 0x04;
  wtr.write<uint8_t>(flags);
  if (isAuto && !isReadable) {
    assert(autoVar.valid());
    wtr.write<PexString>(autoVar);
  } else {
    if (isReadable) {
      assert(readFunction);
      readFunction->write(wtr);
    }
    if (isWritable) {
      assert(writeFunction);
      writeFunction->write(wtr);
    }
  }
}

}}