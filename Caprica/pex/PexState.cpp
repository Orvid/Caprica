#include <pex/PexState.h>

namespace caprica { namespace pex {

void PexState::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.boundWrite<uint16_t>(functions.size());
  for (auto f : functions)
    f->write(wtr);
}

}}