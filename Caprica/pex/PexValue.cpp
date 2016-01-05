#include <pex/PexValue.h>

#include <iomanip>

#include <CapricaError.h>

#include <pex/PexFile.h>

namespace caprica { namespace pex {

void PexValue::writeAsm(const PexFile* file, PexAsmWriter& wtr) const {
  switch (type) {
    case PexValueType::None:
      wtr.write("None");
      break;
    case PexValueType::Identifier:
      wtr.write("%s", file->getStringValue(s).c_str());
      break;
    case PexValueType::String:
      wtr.write("\"%s\"", PexAsmWriter::escapeString(file->getStringValue(s)).c_str());
      break;
    case PexValueType::Integer:
      wtr.write("%i", (int)i);
      break;
    case PexValueType::Float:
      wtr.write("%f", f);
      break;
    case PexValueType::Bool:
      if (b)
        wtr.write("True");
      else
        wtr.write("False");
      break;
    default:
      CapricaError::logicalFatal("Unknown PexValueType to write as asm!");
  }
}

}}
