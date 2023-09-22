#include <pex/PexDebugInfo.h>

namespace caprica { namespace pex {

PexDebugInfo* PexDebugInfo::read(allocators::ChainedPool* alloc, PexReader& rdr, GameID gameType) {
  auto inf = alloc->make<PexDebugInfo>();
  inf->modificationTime = rdr.read<time_t>();
  auto fSize = rdr.read<uint16_t>();
  for (size_t i = 0; i < fSize; i++)
    inf->functions.push_back(PexDebugFunctionInfo::read(alloc, rdr));
  if (gameType == GameID::Skyrim)
    return inf;
  auto pgSize = rdr.read<uint16_t>();
  for (size_t i = 0; i < pgSize; i++)
    inf->propertyGroups.push_back(PexDebugPropertyGroup::read(alloc, rdr));
  auto soSize = rdr.read<uint16_t>();
  for (size_t i = 0; i < soSize; i++)
    inf->structOrders.push_back(PexDebugStructOrder::read(alloc, rdr));
  return inf;
}

void PexDebugInfo::write(PexWriter& wtr, GameID gameType) const {
  wtr.write<time_t>(modificationTime);
  wtr.boundWrite<uint16_t>(functions.size());
  for (auto f : functions)
    f->write(wtr);
  // Skyrim doesn't have the following in debug info
  if (gameType == GameID::Skyrim)
    return;
  wtr.boundWrite<uint16_t>(propertyGroups.size());
  for (auto p : propertyGroups)
    p->write(wtr);
  wtr.boundWrite<uint16_t>(structOrders.size());
  for (auto s : structOrders)
    s->write(wtr);
}

}}
