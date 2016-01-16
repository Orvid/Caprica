#pragma once

#include <string>
#include <vector>

#include <common/CapricaBinaryReader.h>
#include <common/CapricaBinaryWriter.h>

#include <vmad/VMADProperty.h>

namespace caprica { namespace vmad {

enum class VMADScriptStatus : uint8_t
{
  // TODO: Fill out.
  Unknown,
};

struct VMADScript final
{
  std::string name{ "" };
  VMADScriptStatus status{ VMADScriptStatus::Unknown };
  std::vector<VMADProperty*> properties{ };

  static VMADScript* read(CapricaBinaryReader& rdr) {
    auto script = new VMADScript();
    script->name = rdr.read<std::string>();
    script->status = (VMADScriptStatus)rdr.read<uint8_t>();
    auto pCount = rdr.read<uint16_t>();
    for (size_t i = 0; i < pCount; i++)
      script->properties.push_back(VMADProperty::read(rdr));
    return script;
  }

  void write(CapricaBinaryWriter& wtr) const {
    wtr.write<std::string>(name);
    wtr.write<uint8_t>((uint8_t)status);
    wtr.boundWrite<uint16_t>(properties.size());
    for (auto p : properties)
      p->write(wtr);
  }
};

}}
