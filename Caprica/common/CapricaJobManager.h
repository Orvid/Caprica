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

  void await();

protected:
  virtual void run() = 0;

private:
  std::atomic<bool> hasRan{ false };
  std::atomic<bool> runningLock{ false };
  std::condition_variable ranCondition;
  std::mutex ranMutex;

  friend struct CapricaJobManager;
  std::atomic<CapricaJob*> next{ nullptr };

  bool tryRun();
};

struct CapricaJobManager final
{
  void startup(size_t workerCount);
  bool tryDeque(CapricaJob** retJob);
  void queueJob(CapricaJob* job);

  void setQueueInitialized() { queueInitialized.store(true, std::memory_order_relaxed); }
  // Run the currently executing thread as
  // a worker.
  void enjoin();

private:
  struct DefaultJob final : public CapricaJob {
    virtual void run() override { }
  } defaultJob;
  std::atomic<CapricaJob*> front{ &defaultJob };
  std::atomic<CapricaJob*> back{ &defaultJob };
  std::mutex queueAvailabilityMutex;
  std::condition_variable queueCondition;
  std::atomic<size_t> queuedItemCount{ 0 };
  std::atomic<size_t> waiterCount{ 0 };
  std::atomic<size_t> workerCount{ 0 };
  std::atomic<bool> stopWorkers{ false };
  std::atomic<bool> queueInitialized{ false };

  void workerMain();
};

}
