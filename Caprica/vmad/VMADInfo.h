#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <common/CapricaBinaryReader.h>
#include <common/CapricaBinaryWriter.h>

#include <vmad/VMADScript.h>
#include <vmad/VMADFragment.h>

namespace caprica { namespace vmad {

struct VMADInfo final
{
  int16_t version{ 6 };
  int16_t objectFormat{ 2 };
  std::vector<VMADScript*> scripts{ };
  VMADFragment* fragment{ nullptr };

  static VMADInfo* read(CapricaBinaryReader& rdr) {
    auto inf = new VMADInfo();
    inf->version = rdr.read<int16_t>();
    inf->objectFormat = rdr.read<int16_t>();
    auto sCount = rdr.read<uint16_t>();
    for (size_t i = 0; i < sCount; i++)
      inf->scripts.push_back(VMADScript::read(rdr));
    
    if (!rdr.eof()) {
      inf->fragment = QUSTRecordFragment::read(rdr);

    }
    return inf;
  }

  void write(CapricaBinaryWriter& wtr) const {
    wtr.write<int16_t>(version);
    wtr.write<int16_t>(objectFormat);
    wtr.boundWrite<uint16_t>(scripts.size());
    for (auto s : scripts)
      s->write(wtr);
    // TODO: Handle fragments.
  }
};

}}
