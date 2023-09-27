#include <common/CapricaJobManager.h>

#include <common/CapricaReportingContext.h>

namespace caprica {

void CapricaJob::await() {
  if (!tryRun()) {
    std::unique_lock<std::mutex> ranLock { ranMutex };
    ranCondition.wait(ranLock, [this] { return hasRan.load(std::memory_order_consume); });
  }
}
bool CapricaJob::hasRun() {
  return hasRan.load(std::memory_order_acquire);
}
bool CapricaJob::tryRun() {
  if (!hasRan.load(std::memory_order_acquire)) {
    bool r = runningLock.load(std::memory_order_acquire);
    if (!r && runningLock.compare_exchange_strong(r, true)) {
      std::unique_lock<std::mutex> ranLock { ranMutex };
      run();
      hasRan.store(true, std::memory_order_release);
      ranLock.unlock();
      ranCondition.notify_all();
      // We deliberately never release the running lock
    } else {
      return false;
    }
  }
  return true;
}

void CapricaJobManager::startup(size_t initialWorkerCount) {
  defaultJob.await();

  for (size_t i = 0; i < initialWorkerCount; i++) {
    std::thread thr { [this] {
      this->workerMain();
    } };
    thr.detach();
  }
}

void CapricaJobManager::awaitShutdown() {
  std::mutex notARealMutex {};
  while (workerCount != 0) {
    std::unique_lock<std::mutex> lk { notARealMutex };
    queueCondition.wait_for(lk, std::chrono::milliseconds(20), [&] { return workerCount == 0; });
  }
}

bool CapricaJobManager::tryDeque(CapricaJob** retJob) {
  auto fron = front.load(std::memory_order_consume);
  if (!fron)
    return false;
  auto next = fron->next.load(std::memory_order_acquire);
  if (next != nullptr) {
    while (!front.compare_exchange_weak(fron, next)) {
      if (fron == nullptr) {
        // We weren't the ones to put the nullptr there,
        // exit early so that the thread that did put it
        // there can safely put it back.
        return false;
      }
      next = fron->next.load(std::memory_order_acquire);
    }
    if (next == nullptr) {
      // We can only have managed to do this ourselves, nothing else will write
      // while front is still nullptr.
      front.compare_exchange_strong(next, fron);
    } else {
      queuedItemCount--;
    }
  }
  if (!fron->hasRan.load(std::memory_order_consume)) {
    *retJob = fron;
    return true;
  }
  return false;
}

void CapricaJobManager::queueJob(CapricaJob* job) {
  auto oldBack = back.load();
  while (!back.compare_exchange_weak(oldBack, job)) {
  }
  queuedItemCount++;
  oldBack->next.store(job, std::memory_order_release);

  if (waiterCount > 0)
    queueCondition.notify_one();
}

void CapricaJobManager::enjoin() {
  workerMain();
}

void CapricaJobManager::workerMain() {
  std::mutex notARealMutex {};
  const auto waitCallback = [&] {
    return queuedItemCount > 0 || stopWorkers.load(std::memory_order_consume);
  };
  workerCount++;
StartOver:
  CapricaJob* job = nullptr;
  while (tryDeque(&job))
    job->tryRun();

  // Don't stop until all jobs have been added.
  if (stopWorkers.load(std::memory_order_consume)) {
    workerCount--;
    return;
  }

  // If the queue is fully initialized, then only the last
  // living thread is allowed to shut everything down.
  if (!queuedItemCount.load(std::memory_order_consume) && queueInitialized.load(std::memory_order_consume) &&
      waiterCount.load(std::memory_order_consume) == workerCount - 1) {
    stopWorkers.store(true, std::memory_order_release);
    workerCount--;
    queueCondition.notify_all();
    return;
  }

  {
    std::unique_lock<std::mutex> lk { notARealMutex };
    waiterCount++;
    queueCondition.wait_for(lk, std::chrono::milliseconds(100), waitCallback);
    waiterCount--;
    goto StartOver;
  }
}

}
