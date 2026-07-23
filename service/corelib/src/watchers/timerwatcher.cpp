#include "timerwatcher.hpp"

namespace synqueen {

TimerWatcher::TimerWatcher(SharedAsyncPtr checkLocal,
                           SharedAsyncPtr checkRemote, uv_loop_t *loop,
                           int interval)
    : BaseWatcher(checkLocal, checkRemote), loop(loop), interval(interval) {
  timer = new uv_timer_t();
  uv_timer_init(loop, timer);
  timer->data = this;
}

TimerWatcher::~TimerWatcher() {
  if (timer) {
    uv_timer_stop(timer);
    uv_close(reinterpret_cast<uv_handle_t *>(timer), [](uv_handle_t *handle) {
      delete reinterpret_cast<uv_timer_t *>(handle);
    });
    timer = nullptr;
  }
}

void TimerWatcher::startWatch() {
  assert(timer);
  uv_timer_start(
      timer,
      [](uv_timer_t *handle) {
        TimerWatcher *watcher = reinterpret_cast<TimerWatcher *>(handle->data);
        watcher->checkLocalChanges();
      },
      interval, interval);
}

void TimerWatcher::stopWatch() {
  assert(timer);
  uv_timer_stop(timer);
}

} // namespace synqueen
