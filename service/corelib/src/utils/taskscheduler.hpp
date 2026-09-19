#pragma once

#include "utils/corralheader.hpp"
#include "utils/uvutils.hpp"

#include <functional>

namespace synqueen {

// TaskScheduler is a singleton class that allows scheduling functors to run in
// an event loop.
class TaskScheduler {
public:
  static void initialize(uv_loop_t *loop, corral::Nursery *nursery);

  static void runOnMainThread(std::function<void()> task);
  static void runOnMainThreadAsync(std::function<corral::Task<void>()> task);

private:
  TaskScheduler() = default;
  ~TaskScheduler();

  static TaskScheduler *getInstance();

  void initializePrivate(uv_loop_t *loop, corral::Nursery *nursery);

  void runOnMainThreadPrivate(std::function<void()> task);
  void runOnMainThreadAsyncPrivate(std::function<corral::Task<void>()> task);

private:
  static TaskScheduler *self;
  static bool destroyed;

  uv_loop_t *loop;
  corral::Nursery *nursery;
};

} // namespace synqueen
