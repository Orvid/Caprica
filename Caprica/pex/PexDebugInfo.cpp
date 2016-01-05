#include <pex/PexDebugInfo.h>

namespace caprica { namespace pex {

PexDebugInfo* PexDebugInfo::read(PexReader& rdr) {
  auto inf = new PexDebugInfo();
  inf->modificationTime = rdr.read<time_t>();
  auto fSize = rdr.read<uint16_t>();
  inf->functions.reserve(fSize);
  for (size_t i = 0; i < fSize; i++)
    inf->functions.push_back(PexDebugFunctionInfo::read(rdr));
  auto pgSize = rdr.read<uint16_t>();
  inf->propertyGroups.reserve(pgSize);
  for (size_t i = 0; i < pgSize; i++)
    inf->propertyGroups.push_back(PexDebugPropertyGroup::read(rdr));
  auto soSize = rdr.read<uint16_t>();
  inf->structOrders.reserve(soSize);
  for (size_t i = 0; i < soSize; i++)
    inf->structOrders.push_back(PexDebugStructOrder::read(rdr));
  return inf;
}

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
