#pragma once

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include <vmad/VMADScript.h>

namespace caprica { namespace vmad {

struct VMADFragment abstract
{

};

struct QUSTRecordFragment final : public VMADFragment
{
  struct Fragment final
  {
    uint16_t questStage{ };
    int32_t logEntry{ };
    std::string scriptName{ "" };
    std::string fragmentName{ "" };

    static Fragment* read(CapricaBinaryReader& rdr) {
      auto f = new Fragment();
      f->questStage = rdr.read<uint16_t>();
      assert(rdr.read<int16_t>() == 0);
      f->logEntry = rdr.read<int32_t>();
      assert(rdr.read<int8_t>() == 1);
      f->scriptName = rdr.read<std::string>();
      f->fragmentName = rdr.read<std::string>();
      return f;
    }
  };
  struct Alias final
  {
    struct
    {
      uint32_t formID{ };
      uint16_t alias{ };
    } object{ };
    int16_t version{ };
    int16_t objectFormat{ };
    std::vector<VMADScript*> scripts{ };

    static Alias* read(CapricaBinaryReader& rdr) {
      auto a = new Alias();
      auto unused = rdr.read<uint16_t>();
      a->object.alias = rdr.read<uint16_t>();
      a->object.formID = rdr.read<uint32_t>();
      a->version = rdr.read<int16_t>();
      a->objectFormat = rdr.read<int16_t>();
      auto sCount = rdr.read<uint16_t>();
      for (size_t i = 0; i < sCount; i++)
        a->scripts.push_back(VMADScript::read(rdr));
      return a;
    }
  };

  VMADScript* script{ nullptr };
  std::vector<Fragment*> fragments{ };
  std::vector<Alias*> aliases{ };

  static VMADFragment* read(CapricaBinaryReader& rdr) {
    auto frag = new QUSTRecordFragment();
    auto unk = rdr.read<int8_t>();
    assert(unk == 3);
    auto fCount = rdr.read<uint16_t>();
    frag->script = VMADScript::read(rdr);
    for (size_t i = 0; i < fCount; i++)
      frag->fragments.push_back(Fragment::read(rdr));
    auto aCount = rdr.read<uint16_t>();
    for (size_t i = 0; i < aCount; i++)
      frag->aliases.push_back(Alias::read(rdr));
    return frag;
  }
};

}}
