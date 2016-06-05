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
