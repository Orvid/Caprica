#include <pex/PexObject.h>

#include <sstream>

namespace caprica { namespace pex {

void PexObject::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  std::stringstream tmp;
  PexWriter iwtr(tmp);

  iwtr.write<PexString>(parentClassName);
  iwtr.write<PexString>(documentationString);
  iwtr.write<uint8_t>(isConst ? 0x01 : 0x00);
  iwtr.write<PexUserFlags>(userFlags);
  iwtr.write<PexString>(autoStateName);
  
  iwtr.boundWrite<uint16_t>(structs.size());
  for (auto s : structs)
    s->write(iwtr);
  iwtr.boundWrite<uint16_t>(variables.size());
  for (auto v : variables)
    v->write(iwtr);
  iwtr.boundWrite<uint16_t>(properties.size());
  for (auto p : properties)
    p->write(iwtr);
  iwtr.boundWrite<uint16_t>(states.size());
  for (auto s : states)
    s->write(iwtr);

  wtr.boundWrite<uint32_t>(tmp.str().size());
  wtr.writeStream(tmp);
}

}}