#include <pex/PexDebugStructOrder.h>

namespace caprica { namespace pex {

PexDebugStructOrder* PexDebugStructOrder::read(allocators::ChainedPool* alloc, PexReader& rdr) {
  auto ord = alloc->make<PexDebugStructOrder>();
  ord->objectName = rdr.read<PexString>();
  ord->structName = rdr.read<PexString>();
  auto mSize = rdr.read<uint16_t>();
  for (size_t i = 0; i < mSize; i++)
    ord->members.push_back(alloc->make<IntrusivePexString>(rdr.read<PexString>()));
  return ord;
}

void PexDebugStructOrder::write(PexWriter& wtr) const {
  wtr.write<PexString>(objectName);
  wtr.write<PexString>(structName);
  wtr.boundWrite<uint16_t>(members.size());
  for (auto m : members)
    wtr.write<PexString>(*m);
}

}}