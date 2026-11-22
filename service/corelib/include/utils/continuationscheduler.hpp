#pragma once

#include "utils/corralheader.hpp"
#include "utils/taskscheduler.hpp"
#include "utils/uvheader.hpp"
#include "utils/uvutils.hpp"

namespace synqueen::utils {

class ContinuationScheduler {
public:
  static void initialize(uv_loop_t &loop);
  static void schedule(corral::Handle handle);
  static bool isInitialized();

private:
  ContinuationScheduler(uv_loop_t &loop);
  ~ContinuationScheduler();

  static ContinuationScheduler *getInstance();

  void scheduleImpl(corral::Handle handle);
  void processScheduledHandles();

private:
  static ContinuationScheduler *self;
  static bool destroyed;

  TaskScheduler taskScheduler;
  uv_thread_t mainThread;
};

} // namespace synqueen::utils
