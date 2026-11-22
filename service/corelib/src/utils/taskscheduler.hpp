#pragma once

#include "utils/corralheader.hpp"
#include "utils/uvutils.hpp"

#include <functional>

namespace synqueen {

// TaskScheduler allows scheduling functors to run in a given event loop.
class TaskScheduler {
public:
  TaskScheduler(uv_loop_t &loop);
  ~TaskScheduler() = default;

  // Runs a plain task on the main thread
  void runInEventLoop(std::function<void()> task);

protected:
  uv_loop_t &loop;
};

// AsyncTaskScheduler allows scheduling coroutines to run in a given event loop.
class AsyncTaskScheduler : public TaskScheduler {
public:
  AsyncTaskScheduler(uv_loop_t &loop, corral::Nursery &nursery);
  ~AsyncTaskScheduler() = default;

  // Runs a coroutine on the main thread
  void runInEventLoopAsync(std::function<corral::Task<void>()> task);

private:
  corral::Nursery &nursery;
};

} // namespace synqueen
