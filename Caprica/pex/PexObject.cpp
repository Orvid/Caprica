#include <pex/PexObject.h>

#include <sstream>

#include <pex/PexFile.h>

namespace caprica { namespace pex {

PexObject* PexObject::read(PexReader& rdr) {
  auto obj = new PexObject();
  obj->name = rdr.read<PexString>();
  // The size of the object, but we don't care about it.
  rdr.read<uint32_t>();
  obj->parentClassName = rdr.read<PexString>();
  obj->documentationString = rdr.read<PexString>();
  obj->isConst = rdr.read<uint8_t>() != 0;
  obj->userFlags = rdr.read<PexUserFlags>();
  obj->autoStateName = rdr.read<PexString>();
  auto sLen = rdr.read<uint16_t>();
  obj->structs.reserve(sLen);
  for (size_t i = 0; i < sLen; i++)
    obj->structs.push_back(PexStruct::read(rdr));
  auto vLen = rdr.read<uint16_t>();
  obj->variables.reserve(vLen);
  for (size_t i = 0; i < vLen; i++)
    obj->variables.push_back(PexVariable::read(rdr));
  auto pLen = rdr.read<uint16_t>();
  obj->properties.reserve(pLen);
  for (size_t i = 0; i < pLen; i++)
    obj->properties.push_back(PexProperty::read(rdr));
  auto stLen = rdr.read<uint16_t>();
  obj->states.reserve(stLen);
  for (size_t i = 0; i < stLen; i++)
    obj->states.push_back(PexState::read(rdr));
  return obj;
}

void PexObject::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  PexWriter iwtr{ };

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

  auto str = iwtr.getOutputBuffer();
  wtr.boundWrite<uint32_t>(str.size());
  wtr.writeStream(str);
}

void PexObject::writeAsm(const PexFile* file, PexAsmWriter& wtr) const {
  wtr.writeln(".object %s %s", file->getStringValue(name).c_str(), file->getStringValue(parentClassName).c_str());
  wtr.ident++;

  wtr.writeln(".constFlag %i", isConst ? 1 : 0);
  wtr.writeKV<PexUserFlags>("userFlags", userFlags);
  wtr.writeKV<std::string>("docString", file->getStringValue(documentationString));
  wtr.writeln(".autoState %s", file->getStringValue(autoStateName).c_str());
  
  wtr.writeln(".structTable");
  wtr.ident++;
  for (auto s : structs)
    s->writeAsm(file, wtr);
  wtr.ident--;
  wtr.writeln(".endStructTable");

  wtr.writeln(".variableTable");
  wtr.ident++;
  for (auto v : variables)
    v->writeAsm(file, wtr);
  wtr.ident--;
  wtr.writeln(".endVariableTable");

  wtr.writeln(".propertyTable");
  wtr.ident++;
  for (auto p : properties)
    p->writeAsm(file, this, wtr);
  wtr.ident--;
  wtr.writeln(".endPropertyTable");

  wtr.writeln(".stateTable");
  wtr.ident++;
  for (auto s : states)
    s->writeAsm(file, this, wtr);
  wtr.ident--;
  wtr.writeln(".endStateTable");

  wtr.ident--;
  wtr.writeln(".endObject");
}

}}
