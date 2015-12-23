#include <pex/PexInstruction.h>

#include <limits>

namespace caprica { namespace pex {

void PexInstruction::write(PexWriter& wtr) const {
  wtr.write<uint8_t>((uint8_t)opCode);
  for (auto& a : args)
    wtr.write<PexValue>(a);

  switch (opCode) {
    case PexOpCode::CallMethod:
    case PexOpCode::CallParent:
    case PexOpCode::CallStatic:
    {
      assert(variadicArgs.size() <= std::numeric_limits<uint32_t>::max());
      PexValue val{ };
      val.type = PexValueType::Integer;
      val.i = (uint32_t)variadicArgs.size();
      wtr.write<PexValue>(val);
      for (auto& v : variadicArgs)
        wtr.write<PexValue>(v);
      break;
    }
    default:
      assert(variadicArgs.size() == 0);
      break;
  }
}

}}