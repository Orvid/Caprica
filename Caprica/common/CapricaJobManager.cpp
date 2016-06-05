#include <common/CapricaJobManager.h>

namespace caprica {

void CapricaJob::await() {
  if (!hasRan.load(std::memory_order_acquire)) {
    bool r = runningLock.load(std::memory_order_acquire);
    if (!r && runningLock.compare_exchange_strong(r, true)) {
      std::unique_lock<std::mutex> ranLock{ ranMutex };
      run();
      hasRan.store(true, std::memory_order_release);
      ranLock.unlock();
      ranCondition.notify_all();
      // We deliberately never release the running lock
    } else {
      std::unique_lock<std::mutex> ranLock{ ranMutex };
      ranCondition.wait(ranLock, [this] { return hasRan.load(std::memory_order_consume); });
    }
  }
}

void CapricaJobManager::startup(size_t workerCount) {
  defaultJob.await();

  for (size_t i = 0; i < workerCount; i++) {
    std::thread thr{ [this] { this->workerMain(); } };
    thr.detach();
  }
}

bool CapricaJobManager::tryDeque(CapricaJob** retJob) {
  auto fron = front.load(std::memory_order_consume);
  if (fron != nullptr && fron->next.load(std::memory_order_acquire) != nullptr) {
    CapricaJob* prevFron = nullptr;
    while (fron && !front.compare_exchange_weak(fron, fron->next)) {
      prevFron = fron;
      if (fron == nullptr) {
        // We weren't the ones to put the nullptr there,
        // exit early so that the thread that did put it
        // there can safely put it back.
        return false;
      }
    }
    if (fron == nullptr) {
      assert(prevFron != nullptr);
      // We can only have managed to do this ourselves, nothing else will write
      // while front is still nullptr.
      front.compare_exchange_weak(fron, prevFron);
      return false;
    }
    *retJob = fron;
    return true;
  }
  return false;
}

void CapricaJobManager::queueJob(CapricaJob* job) {
  auto oldBack = back.load();
  while (!back.compare_exchange_weak(oldBack, job)) { }
  oldBack->next.store(job, std::memory_order_release);

  if (waiterCount > 0)
    queueCondition.notify_one();
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
