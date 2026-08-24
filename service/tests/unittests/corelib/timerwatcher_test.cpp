#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <thread>

#include "corelib/src/watchers/timerwatcher.hpp"
#include "utils/uvutils.hpp"

using namespace testing;
using namespace std;
using namespace synqueen;

TEST(TimerWatcherTest, CtorDtor) {
  uv_loop_t loop;
  EXPECT_EQ(uv_loop_init(&loop), 0);
  auto checkLocal = SharedAsyncPtr(new uv_async_t(), deleteHandle<uv_async_t>);
  auto checkRemote = SharedAsyncPtr(new uv_async_t(), deleteHandle<uv_async_t>);
  EXPECT_EQ(uv_async_init(&loop, checkLocal.get(), [](uv_async_t *handle) {}),
            0);
  EXPECT_EQ(uv_async_init(&loop, checkRemote.get(), [](uv_async_t *handle) {}),
            0);
  {
    auto watcher =
        make_unique<TimerWatcher>(checkLocal, checkRemote, &loop, 10);
  }
  checkLocal.reset();
  checkRemote.reset();
  // Just to close handles properly
  uv_run(&loop, UV_RUN_DEFAULT);
  EXPECT_EQ(uv_loop_close(&loop), 0);
}

TEST(TimerWatcherTest, StartStop) {
  uv_loop_t loop;
  EXPECT_EQ(uv_loop_init(&loop), 0);
  auto checkLocal = SharedAsyncPtr(new uv_async_t(), deleteHandle<uv_async_t>);
  auto checkRemote = SharedAsyncPtr(new uv_async_t(), deleteHandle<uv_async_t>);
  const auto interval = 10;
  int counter = 0;
  EXPECT_EQ(uv_async_init(&loop, checkLocal.get(),
                          [](uv_async_t *handle) {
                            auto counter_ptr =
                                reinterpret_cast<int *>(handle->data);
                            (*counter_ptr)++;
                          }),
            0);
  checkLocal->data = &counter;
  EXPECT_EQ(uv_async_init(&loop, checkRemote.get(),
                          [](uv_async_t *handle) {
                            // Do nothing for remote check at this moment
                          }),
            0);
  {
    auto watcher =
        new TimerWatcher(checkLocal, checkRemote, &loop, 0, interval);
    uv_timer_t stopTimer;
    uv_timer_init(&loop, &stopTimer);
    watcher->startWatch();
    struct TimerData {
      TimerWatcher *watcher;
      uv_timer_t *stopTimer;
      SharedAsyncPtr checkLocal;
      SharedAsyncPtr checkRemote;
      uv_loop_t *loop;
    };
    TimerData timerData{watcher, &stopTimer, std::move(checkLocal),
                        std::move(checkRemote), &loop};
    stopTimer.data = &timerData;

    // Starting a timer to stop the watcher
    uv_timer_start(
        &stopTimer,
        [](uv_timer_t *handle) {
          TimerData *timerData = reinterpret_cast<TimerData *>(handle->data);
          TimerWatcher *watcher = timerData->watcher;
          watcher->stopWatch();
          delete watcher;

          timerData->checkLocal.reset();
          timerData->checkRemote.reset();
          uv_close(reinterpret_cast<uv_handle_t *>(timerData->stopTimer),
                   nullptr);
        },
        100, 0);
    uv_run(&loop, UV_RUN_DEFAULT);

    // We don't care too much about accuracy, just make sure that it repeats
    EXPECT_GT(counter, 2);
  }
  EXPECT_EQ(uv_loop_close(&loop), 0);
}
