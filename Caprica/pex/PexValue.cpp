#include <pex/PexValue.h>

#include <common/CapricaReportingContext.h>

#include <pex/PexFile.h>

namespace caprica { namespace pex {

void PexValue::writeAsm(const PexFile* file, PexAsmWriter& wtr) const {
  switch (type) {
    case PexValueType::None:
      wtr.write("None");
      return;
    case PexValueType::Identifier:
      wtr.write("%s", file->getStringValue(s).c_str());
      return;
    case PexValueType::String:
      wtr.write("\"%s\"", PexAsmWriter::escapeString(file->getStringValue(s)).c_str());
      return;
    case PexValueType::Integer:
      wtr.write("%i", (int)i);
      return;
    case PexValueType::Float:
      wtr.write("%f", f);
      return;
    case PexValueType::Bool:
      if (b)
        wtr.write("True");
      else
        wtr.write("False");
      return;
    case PexValueType::Label:
    case PexValueType::TemporaryVar:
    case PexValueType::Invalid:
      break;
  }
  CapricaReportingContext::logicalFatal("Unknown PexValueType to write as asm!");
}

bool PexValue::operator ==(const PexValue& other) const {
  if (type != other.type)
    return false;

  switch (type) {
    case PexValueType::None:
      return true;
    case PexValueType::String:
    case PexValueType::Identifier:
      return s.index == other.s.index;
    case PexValueType::Integer:
      return i == other.i;
    case PexValueType::Float:
      return f == other.f;
    case PexValueType::Bool:
      return b == other.b;
    case PexValueType::Label:
    case PexValueType::TemporaryVar:
    case PexValueType::Invalid:
      break;
  }
  CapricaReportingContext::logicalFatal("Unknown PexValueType to compare!");
}

}}
