#include <pex/PexDebugInfo.h>

namespace caprica { namespace pex {

void PexDebugInfo::write(PexWriter& wtr) const {
  wtr.write<time_t>(modificationTime);
  wtr.boundWrite<uint16_t>(functions.size());
  for (auto f : functions)
    f->write(wtr);
  wtr.boundWrite<uint16_t>(propertyGroups.size());
  for (auto p : propertyGroups)
    p->write(wtr);
  wtr.boundWrite<uint16_t>(structOrders.size());
  for (auto s : structOrders)
    s->write(wtr);
}

}}