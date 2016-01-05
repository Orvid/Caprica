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

void PexFile::writeAsm(PexAsmWriter& wtr) const {
  wtr.writeln(".info");
  wtr.ident++;
  wtr.writeKV<std::string>("source", sourceFileName);
  if (debugInfo)
    wtr.writeKV<time_t>("modifyTime", debugInfo->modificationTime);
  else
    wtr.writeln(".modifyTime 0 ;Debug info: No");
  wtr.writeKV<time_t>("compileTime", compilationTime);
  wtr.writeKV<std::string>("user", userName);
  wtr.writeKV<std::string>("computer", computerName);
  wtr.ident--;
  wtr.writeln(".endInfo");

  wtr.writeln(".userFlagsRef");
  wtr.ident++;
  for (auto a : userFlagTable)
    wtr.writeln(".flag %s %u", getStringValue(a.first).c_str(), (unsigned)a.second);
  wtr.ident--;
  wtr.writeln(".endUserFlagsRef");

  wtr.writeln(".objectTable");
  wtr.ident++;
  for (auto obj : objects)
    obj->writeAsm(this, wtr);
  wtr.ident--;
  // Interestingly, the CK -keepasm option doesn't including a newline after
  // the object table, although we'll choose to put one here.
  wtr.writeln(".endObjectTable");
}

}}
