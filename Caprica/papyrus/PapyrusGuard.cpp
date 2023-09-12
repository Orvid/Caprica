#include <papyrus/PapyrusGuard.h>
#include <papyrus/PapyrusObject.h>
namespace caprica { namespace papyrus {
void PapyrusGuard::buildPex(caprica::CapricaReportingContext &, caprica::pex::PexFile *file,
                            caprica::pex::PexObject *obj) const {
  auto guard = file->alloc->make<pex::PexGuard>();
  guard->name = file->getString(name);
  obj->guards.push_back(guard);
}


void PapyrusGuard::semantic2(PapyrusResolutionContext *ctx) {
  // TODO: check if this is possible or not in Starfield
//  if (ctx->object->isNative())
//    ctx->reportingContext.error(location, "You cannot define guards in a Native script.");
}


}}