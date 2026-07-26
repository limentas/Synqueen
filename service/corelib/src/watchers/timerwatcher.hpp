#pragma once

#include "basewatcher.hpp"

#include "utils/uvutils.hpp"

namespace synqueen {

// Requests local check with a regular interval
class TimerWatcher : public BaseWatcher {
public:
  TimerWatcher(SharedAsyncPtr checkLocal, SharedAsyncPtr checkRemote,
               uv_loop_t *loop, int startDelay = startDelayInterval,
               int interval = checkInterval);
  virtual ~TimerWatcher() override;

  void startWatch() override;
  void stopWatch() override;

private:
  static const int startDelayInterval = 100;       // 100 milliseconds
  static const int checkInterval = 10 * 60 * 1000; // 10 minutes

  int startDelay = startDelayInterval;
  int interval = checkInterval;
  uv_loop_t *loop = nullptr;
  uv_timer_t *timer = nullptr;
};

} // namespace synqueen
