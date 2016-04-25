#include <pex/PexFile.h>

#include <fstream>
#include <iostream>

#include <common/CapricaReportingContext.h>

namespace caprica { namespace pex {

PexFile* PexFile::read(PexReader& rdr) {
  auto file = new PexFile();
  if (rdr.read<uint32_t>() != 0xFA57C0DE)
    CapricaReportingContext::logicalFatal("Unrecognized magic number!");
  if ((file->majorVersion = rdr.read<uint8_t>()) != 3)
    CapricaReportingContext::logicalFatal("We currently only support major version 3!");
  if ((file->minorVersion = rdr.read<uint8_t>()) != 9)
    CapricaReportingContext::logicalFatal("We currently only support minor version 9!");
  file->gameID = rdr.read<uint16_t>();
  file->compilationTime = rdr.read<time_t>();
  file->sourceFileName = rdr.read<std::string>();
  file->userName = rdr.read<std::string>();
  file->computerName = rdr.read<std::string>();

  auto strTableSize = rdr.read<uint16_t>();
  file->stringTable.reserve(strTableSize);
  for (size_t i = 0; i < strTableSize; i++) {
    auto str = rdr.read<std::string>();
    file->stringTable.push_back(str);
    file->stringTableLookup[str] = i;
  }

  if (rdr.read<uint8_t>() != 0)
    file->debugInfo = PexDebugInfo::read(rdr);

  auto ufTableSize = rdr.read<uint16_t>();
  file->userFlagTable.reserve(ufTableSize);
  for (size_t i = 0; i < ufTableSize; i++) {
    auto str = rdr.read<PexString>();
    auto v = rdr.read<uint8_t>();
    file->userFlagTable.push_back(std::make_pair(str, v));
    file->userFlagTableLookup[str.index] = i;
  }

  auto objTableSize = rdr.read<uint16_t>();
  file->objects.reserve(objTableSize);
  for (size_t i = 0; i < objTableSize; i++)
    file->objects.push_back(PexObject::read(rdr));

  return file;
}

void PexFile::write(PexWriter& wtr) const {
  wtr.write<uint32_t>(0xFA57C0DE); // Magic Number
  wtr.write<uint8_t>(majorVersion);
  wtr.write<uint8_t>(minorVersion);
  wtr.write<uint16_t>(gameID);
  wtr.write<time_t>(compilationTime);
  wtr.write<const std::string&>(sourceFileName);
  wtr.write<const std::string&>(userName);
  wtr.write<const std::string&>(computerName);

  wtr.boundWrite<uint16_t>(stringTable.size());
  for (auto& str : stringTable)
    wtr.write<const std::string&>(str);

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
