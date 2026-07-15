#pragma once

#include "basewatcher.hpp"

namespace synqueen {

// Requests local check with a regular interval
class TimerWatcher : public BaseWatcher {
public:
  TimerWatcher(uv_async_t *checkLocal, uv_async_t *checkRemote, uv_loop_t *loop,
               int interval = checkInterval);
  virtual ~TimerWatcher() override;

  void startWatch() override;
  void stopWatch() override;

private:
  static const int checkInterval = 5000; // 5 seconds

  int interval = checkInterval;
  uv_loop_t *loop = nullptr;
  uv_timer_t *timer = nullptr;
};

} // namespace synqueen
