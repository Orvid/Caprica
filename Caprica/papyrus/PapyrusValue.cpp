#include <papyrus/PapyrusValue.h>

#include <papyrus/PapyrusResolutionContext.h>

namespace caprica { namespace papyrus {

PapyrusValue PapyrusValue::defaultFromType(PapyrusResolutionContext* ctx, const PapyrusType& tp) {
  switch (tp.type) {
    case PapyrusType::Kind::String:
      return PapyrusValue::String(tp.location, "");
    case PapyrusType::Kind::Int:
      return PapyrusValue::Integer(tp.location, 0);
    case PapyrusType::Kind::Float:
      return PapyrusValue::Float(tp.location, 0.0f);
    case PapyrusType::Kind::Bool:
      return PapyrusValue::Bool(tp.location, false);
    default:
      ctx->reportingContext.error(tp.location, "Unable to generate a default value of type '{}'.", tp);
      return PapyrusValue::None(tp.location);
  }
}

}}
