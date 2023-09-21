#include <pex/PexProperty.h>

#include <pex/PexDebugFunctionInfo.h>
#include <pex/PexFile.h>
#include <pex/PexObject.h>

namespace caprica { namespace pex {

PexProperty * PexProperty::read(allocators::ChainedPool *alloc, PexReader &rdr, GameID gameType) {
  auto prop = alloc->make<PexProperty>();
  prop->name = rdr.read<PexString>();
  prop->typeName = rdr.read<PexString>();
  prop->documentationString = rdr.read<PexString>();
  prop->userFlags = rdr.read<PexUserFlags>();
  auto flags = rdr.read<uint8_t>();
  if (flags & 0x01)
    prop->isReadable = true;
  if (flags & 0x02)
    prop->isWritable = true;
  if (flags & 0x04)
    prop->isAuto = true;

  if (prop->isAuto) {
    prop->autoVar = rdr.read<PexString>();
  } else {
    if (prop->isReadable)
      prop->readFunction = PexFunction::read(alloc, rdr, true, gameType);
    if (prop->isWritable)
      prop->writeFunction = PexFunction::read(alloc, rdr, true, gameType);
  }
  return prop;
}

void PexProperty::write(PexWriter& wtr) const {
  wtr.write<PexString>(name);
  wtr.write<PexString>(typeName);
  wtr.write<PexString>(documentationString);
  wtr.write<PexUserFlags>(userFlags);
  uint8_t flags = 0;
  if (isReadable)
    flags |= 0x01;
  if (isWritable)
    flags |= 0x02;
  if (isAuto)
    flags |= 0x04;
  wtr.write<uint8_t>(flags);
  if (isAuto) {
    assert(autoVar.valid());
    wtr.write<PexString>(autoVar);
  } else {
    if (isReadable) {
      assert(readFunction);
      readFunction->write(wtr);
    }
    if (isWritable) {
      assert(writeFunction);
      writeFunction->write(wtr);
    }
  }
}

void PexProperty::writeAsm(const PexFile* file, const PexObject* obj, PexAsmWriter& wtr) const {
  // TODO: Handle the property group info in the debug info.
  wtr.write(".property %s %s", file->getStringValue(name).to_string().c_str(), file->getStringValue(typeName).to_string().c_str());
  if (isAuto)
    wtr.write(" auto");
  wtr.writeln();
  wtr.ident++;

  wtr.writeKV<PexUserFlags>("userFlags", userFlags);
  wtr.writeKV<std::string>("docString", file->getStringValue(documentationString).to_string());
  if (isAuto) {
    wtr.writeln(".autoVar %s", file->getStringValue(autoVar).to_string().c_str());
  } else {
    if (isReadable)
      readFunction->writeAsm(file, obj, nullptr, PexDebugFunctionType::Getter, file->getStringValue(name).to_string(), wtr);
    if (isWritable)
      writeFunction->writeAsm(file, obj, nullptr, PexDebugFunctionType::Setter, file->getStringValue(name).to_string(), wtr);
  }

  wtr.ident--;
  wtr.writeln(".endProperty");
}

}}
