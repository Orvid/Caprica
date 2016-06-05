#pragma once

#include <cassert>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace caprica {

struct CapricaJob abstract
{
  CapricaJob() = default;
  CapricaJob(const CapricaJob& other) = delete;
  CapricaJob(CapricaJob&& other) = delete;
  CapricaJob& operator =(const CapricaJob&) = delete;
  CapricaJob& operator =(CapricaJob&&) = delete;
  ~CapricaJob() = default;

  void await() {
    if (!hasRan.load(std::memory_order_acquire)) {
      bool r = runningLock.load(std::memory_order_acquire);
      if (!r && runningLock.compare_exchange_strong(r, true)) {
        std::unique_lock<std::mutex> ranLock{ ranMutex };
        run();
        hasRan.store(true, std::memory_order_release);
        ranLock.unlock();
        ranCondition.notify_all();
        // We deliberately never release the running lock,
        // so we just use it as a once_flag.
      } else {
        std::unique_lock<std::mutex> ranLock{ ranMutex };
        ranCondition.wait(ranLock, [this] { return hasRan.load(std::memory_order_consume); });
      }
    }
  }

protected:
  virtual void run() abstract = 0;

private:
  std::atomic<bool> hasRan{ false };
  std::condition_variable ranCondition;
  std::mutex ranMutex;
  std::atomic<bool> runningLock{ false };

  friend struct CapricaJobManager;
  std::atomic<CapricaJob*> next{ nullptr };
};

struct CapricaJobManager final
{
  void startup(size_t workerCount);
  bool tryDeque(CapricaJob** retJob);
  void queueJob(CapricaJob* job);

private:
  struct DefaultJob final : public CapricaJob {
    virtual void run() override { }
  } defaultJob;
  std::atomic<CapricaJob*> front{ &defaultJob };
  std::atomic<CapricaJob*> back{ &defaultJob };
  std::mutex queueAvailabilityMutex;
  std::condition_variable queueCondition;
  std::atomic<size_t> waiterCount{ 0 };
  std::atomic<bool> stopWorkers{ false };

  void workerMain();
};

}
