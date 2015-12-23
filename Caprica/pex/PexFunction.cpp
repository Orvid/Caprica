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

  wtr.boundWrite<uint16_t>(parameters.size());
  for (auto p : parameters)
    p->write(wtr);
  wtr.boundWrite<uint16_t>(locals.size());
  for (auto l : locals)
    l->write(wtr);
  wtr.boundWrite<uint16_t>(instructions.size());
  for (auto i : instructions)
    i->write(wtr);
}

}}