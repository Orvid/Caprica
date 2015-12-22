#include <pex/PexFunction.h>

namespace caprica { namespace pex {

void PexFunction::write(PexWriter& wtr) const {
  if (name.valid())
    wtr.write<PexString>(name);
  wtr.write<PexString>(returnTypeName);
  wtr.write<PexString>(documenationString);
  wtr.write<PexUserFlags>(userFlags);
  uint8_t flags = 0;
  if (isGlobal)
    flags |= 1;
  if (isNative)
    flags |= 2;
  wtr.write<uint8_t>(flags);
  // Todo: The rest.
}

}}