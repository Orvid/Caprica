#include <pex/PexDebugStructOrder.h>

namespace caprica { namespace pex {

PexDebugStructOrder* PexDebugStructOrder::read(PexReader& rdr) {
  auto ord = new PexDebugStructOrder();
  ord->objectName = rdr.read<PexString>();
  ord->structName = rdr.read<PexString>();
  auto mSize = rdr.read<uint16_t>();
  ord->members.reserve(mSize);
  for (size_t i = 0; i < mSize; i++)
    ord->members.push_back(rdr.read<PexString>());
  return ord;
}

void PexDebugStructOrder::write(PexWriter& wtr) const {
  wtr.write<PexString>(objectName);
  wtr.write<PexString>(structName);
  wtr.boundWrite<uint16_t>(members.size());
  for (auto m : members)
    wtr.write<PexString>(m);
}

}}