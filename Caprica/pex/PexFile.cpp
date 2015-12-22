#include <pex/PexFile.h>

#include <fstream>
#include <iostream>

namespace caprica { namespace pex {

void PexFile::writeToFile(PexWriter& wtr) {
  wtr.write<uint32_t>(0xFA57C0DE); // Magic Number
  wtr.write<uint8_t>(majorVersion);
  wtr.write<uint8_t>(minorVersion);
  wtr.write<uint16_t>(gameID);
  wtr.write<time_t>(compilationTime);
  wtr.write<std::string>(sourceFileName);
  wtr.write<std::string>(userName);
  wtr.write<std::string>(computerName);

  assert(stringTable.size() <= std::numeric_limits<uint16_t>::max());
  wtr.write<uint16_t>((uint16_t)stringTable.size());
  for (auto& str : stringTable)
    wtr.write<std::string>(str);
}

}}