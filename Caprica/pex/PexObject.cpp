#include <pex/PexObject.h>

#include <sstream>

#include <pex/PexFile.h>

namespace caprica { namespace pex {

PexObject* PexObject::read(allocators::ChainedPool* alloc, PexReader& rdr) {
  auto obj = alloc->make<PexObject>();
  obj->name = rdr.read<PexString>();
  // The size of the object, but we don't care about it.
  rdr.read<uint32_t>();
  obj->parentClassName = rdr.read<PexString>();
  obj->documentationString = rdr.read<PexString>();
  obj->isConst = rdr.read<uint8_t>() != 0;
  obj->userFlags = rdr.read<PexUserFlags>();
  obj->autoStateName = rdr.read<PexString>();
  auto sLen = rdr.read<uint16_t>();
  for (size_t i = 0; i < sLen; i++)
    obj->structs.push_back(PexStruct::read(alloc, rdr));
  auto vLen = rdr.read<uint16_t>();
  for (size_t i = 0; i < vLen; i++)
    obj->variables.push_back(PexVariable::read(alloc, rdr));
  auto pLen = rdr.read<uint16_t>();
  for (size_t i = 0; i < pLen; i++)
    obj->properties.push_back(PexProperty::read(alloc, rdr));
  auto stLen = rdr.read<uint16_t>();
  for (size_t i = 0; i < stLen; i++)
    obj->states.push_back(PexState::read(alloc, rdr));
  return obj;
}

void PexObject::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.beginObject();

  wtr.write<PexString>(parentClassName);
  wtr.write<PexString>(documentationString);
  wtr.write<uint8_t>(isConst ? 0x01 : 0x00);
  wtr.write<PexUserFlags>(userFlags);
  wtr.write<PexString>(autoStateName);
  
  wtr.boundWrite<uint16_t>(structs.size());
  for (auto s : structs)
    s->write(wtr);
  wtr.boundWrite<uint16_t>(variables.size());
  for (auto v : variables)
    v->write(wtr);
  wtr.boundWrite<uint16_t>(properties.size());
  for (auto p : properties)
    p->write(wtr);
  wtr.boundWrite<uint16_t>(states.size());
  for (auto s : states)
    s->write(wtr);
  wtr.endObject();
}

void PexObject::writeAsm(const PexFile* file, PexAsmWriter& wtr) const {
  wtr.write(".object %s %s", file->getStringValue(name).to_string().c_str(), file->getStringValue(parentClassName).to_string().c_str());
  if (isConst)
    wtr.write(" const");
  wtr.writeln();
  wtr.ident++;

  wtr.writeKV<PexUserFlags>("userFlags", userFlags);
  wtr.writeKV<std::string>("docString", file->getStringValue(documentationString).to_string());
  wtr.writeln(".autoState %s", file->getStringValue(autoStateName).to_string().c_str());
  
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

  wtr.writeln(".propertyGroupTable");
  wtr.ident++;
  if (file->debugInfo) {
    for (auto g : file->debugInfo->propertyGroups) {
      if (file->getStringValue(g->objectName) == file->getStringValue(name)) {
        g->writeAsm(file, wtr);
      }
    }
  }
  wtr.ident--;
  wtr.writeln(".endPropertyGroupTable");

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
