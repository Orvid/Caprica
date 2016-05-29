#include <common/CapricaJobManager.h>

namespace caprica {

void CapricaJobManager::startup() {
  defaultJob.await();
}

void someTestFunc(CapricaJobManager* mgr, CapricaJob* job) {
  mgr->queueJob(job);
}

}
