#include <common/CapricaJobManager.h>

namespace caprica {

void CapricaJobManager::startup() {
  defaultJob.await();

  for (size_t i = 0; i < workerCount; i++) {
    std::thread thr{ [this] { this->workerMain(); } };
    thr.detach();
  }
}

void CapricaJobManager::workerMain() {
StartOver:
  CapricaJob* job = nullptr;
  while (tryDeque(&job)) {
    job->await();
  }
  // Don't stop until all jobs have run.
  if (stopWorkers.load(std::memory_order_consume))
    return;

  {
    std::unique_lock<std::mutex> lk{ queueAvailabilityMutex };
    waiterCount++;
    queueCondition.wait(lk);
    waiterCount--;
    goto StartOver;
  }
}

}
