#include <pex/PexProperty.h>

#include <pex/PexDebugFunctionInfo.h>
#include <pex/PexFile.h>
#include <pex/PexObject.h>

namespace caprica { namespace pex {

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
  wtr.write(".property %s %s", file->getStringValue(name).c_str(), file->getStringValue(typeName).c_str());
  if (isAuto)
    wtr.write(" auto");
  wtr.writeln();
  wtr.ident++;

  wtr.writeKV<PexUserFlags>("userFlags", userFlags);
  wtr.writeKV<std::string>("docString", file->getStringValue(documentationString));
  if (isAuto) {
    wtr.writeln(".autoVar %s", file->getStringValue(autoVar).c_str());
  } else {
    if (isReadable)
      readFunction->writeAsm(file, obj, nullptr, PexDebugFunctionType::Getter, file->getStringValue(name), wtr);
    if (isWritable)
      writeFunction->writeAsm(file, obj, nullptr, PexDebugFunctionType::Setter, file->getStringValue(name), wtr);
  }

  wtr.ident--;
  wtr.writeln(".endProperty");
}

}}
