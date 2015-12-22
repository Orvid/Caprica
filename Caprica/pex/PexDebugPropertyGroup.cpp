#include <pex/PexDebugPropertyGroup.h>

namespace caprica { namespace pex {

void PexDebugPropertyGroup::write(PexWriter& wtr) const {
  wtr.write<PexString>(objectName);
  wtr.write<PexString>(groupName);
  wtr.write<PexString>(documentationString);
  wtr.write<PexUserFlags>(userFlags);
  wtr.boundWrite<uint16_t>(properties.size());
  for (auto p : properties)
    wtr.write<PexString>(p);
}

}}