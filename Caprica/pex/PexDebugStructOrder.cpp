#include <pex/PexDebugStructOrder.h>

namespace caprica { namespace pex {

void PexDebugStructOrder::write(PexWriter& wtr) const {
  wtr.write<PexString>(objectName);
  wtr.write<PexString>(structName);
  wtr.boundWrite<uint16_t>(members.size());
  for (auto m : members)
    wtr.write<PexString>(m);
}

}}