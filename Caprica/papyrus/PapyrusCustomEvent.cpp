#include <papyrus/PapyrusCustomEvent.h>

#include <papyrus/PapyrusObject.h>

namespace caprica { namespace papyrus {

void PapyrusCustomEvent::semantic2(PapyrusResolutionContext* ctx) {
  if (ctx->object->isNative()) {
    ctx->reportingContext.error(location, "Scripts marked as Native are not allowed to define CustomEvents.");
  }
}

}}
