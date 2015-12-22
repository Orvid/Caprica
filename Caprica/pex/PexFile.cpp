#include <pex/PexFile.h>

#include <fstream>
#include <iostream>

namespace caprica { namespace pex {

void PexFile::write(PexWriter& wtr) const {
  wtr.write<uint32_t>(0xFA57C0DE); // Magic Number
  wtr.write<uint8_t>(majorVersion);
  wtr.write<uint8_t>(minorVersion);
  wtr.write<uint16_t>(gameID);
  wtr.write<time_t>(compilationTime);
  wtr.write<std::string>(sourceFileName);
  wtr.write<std::string>(userName);
  wtr.write<std::string>(computerName);

  wtr.boundWrite<uint16_t>(stringTable.size());
  for (auto& str : stringTable)
    wtr.write<std::string>(str);

  if (debugInfo) {
    wtr.write<uint8_t>(0x01);
    debugInfo->write(wtr);
  } else {
    wtr.write<uint8_t>(0x00);
  }

  wtr.boundWrite<uint16_t>(userFlagTable.size());
  for (auto uf : userFlagTable) {
    wtr.write<PexString>(uf.first);
    wtr.write<uint8_t>(uf.second);
  }

  wtr.boundWrite<uint16_t>(objects.size());
  for (auto o : objects)
    o->write(wtr);
}

}}