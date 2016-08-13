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
      wtr.write("%s", file->getStringValue(val.s).to_string().c_str());
      return;
    case PexValueType::String:
      wtr.write("\"%s\"", PexAsmWriter::escapeString(file->getStringValue(val.s).to_string()).c_str());
      return;
    case PexValueType::Integer:
      wtr.write("%i", (int)val.i);
      return;
    case PexValueType::Float:
      wtr.write("%f", val.f);
      return;
    case PexValueType::Bool:
      if (val.b)
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
      return val.s.index == other.val.s.index;
    case PexValueType::Integer:
      return val.i == other.val.i;
    case PexValueType::Float:
      return val.f == other.val.f;
    case PexValueType::Bool:
      return val.b == other.val.b;
    case PexValueType::Label:
    case PexValueType::TemporaryVar:
    case PexValueType::Invalid:
      break;
  }
  CapricaReportingContext::logicalFatal("Unknown PexValueType to compare!");
}

}}
