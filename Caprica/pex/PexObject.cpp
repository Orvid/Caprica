#include <pex/PexObject.h>

#include <sstream>

#include <pex/PexFile.h>

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
