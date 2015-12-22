#include <pex/PexStructInfo.h>

namespace caprica { namespace pex {

void PexStructInfo::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.boundWrite<uint16_t>(members.size());
  for (auto m : members)
    m->write(wtr);
}

}}