#include <pex/PexDebugPropertyGroup.h>

#include <pex/PexFile.h>

namespace caprica { namespace pex {

PexDebugPropertyGroup* PexDebugPropertyGroup::read(PexReader& rdr) {
  auto group = new PexDebugPropertyGroup();
  group->objectName = rdr.read<PexString>();
  group->groupName = rdr.read<PexString>();
  group->documentationString = rdr.read<PexString>();
  group->userFlags = rdr.read<PexUserFlags>();
  auto pSize = rdr.read<uint16_t>();
  group->properties.reserve(pSize);
  for (size_t i = 0; i < pSize; i++)
    group->properties.push_back(rdr.read<PexString>());
  return group;
}

void PexDebugPropertyGroup::write(PexWriter& wtr) const {
  wtr.write<PexString>(objectName);
  wtr.write<PexString>(groupName);
  wtr.write<PexString>(documentationString);
  wtr.write<PexUserFlags>(userFlags);
  wtr.boundWrite<uint16_t>(properties.size());
  for (auto p : properties)
    wtr.write<PexString>(p);
}

void PexDebugPropertyGroup::writeAsm(const PexFile* file, PexAsmWriter& wtr) const {
  wtr.writeln(".propertyGroup %s", file->getStringValue(groupName).to_string().c_str());
  wtr.ident++;

  wtr.writeKV<PexUserFlags>("userFlags", userFlags);
  wtr.writeKV<std::string>("docString", file->getStringValue(documentationString).to_string());
  for (auto p : properties)
    wtr.writeln(".property %s", file->getStringValue(p).to_string().c_str());

  wtr.ident--;
  wtr.writeln(".endPropertyGroup");
}

}}
