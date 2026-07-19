#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <thread>

#include "corelib/include/utils/uvutils.hpp"
#include "corelib/src/watchers/timerwatcher.hpp"

using namespace testing;
using namespace std;
using namespace synqueen;

TEST(TimerWatcherTest, CtorDtor) {
  uv_loop_t loop;
  EXPECT_EQ(uv_loop_init(&loop), 0);
  uv_async_t checkLocal;
  uv_async_t checkRemote;
  EXPECT_EQ(uv_async_init(&loop, &checkLocal, [](uv_async_t *handle) {}), 0);
  EXPECT_EQ(uv_async_init(&loop, &checkRemote, [](uv_async_t *handle) {}), 0);
  {
    auto watcher =
        make_unique<TimerWatcher>(&checkLocal, &checkRemote, &loop, 10);
  }
  uv_close(reinterpret_cast<uv_handle_t *>(&checkLocal), nullptr);
  uv_close(reinterpret_cast<uv_handle_t *>(&checkRemote), nullptr);
  // Just to close handles properly
  uv_run(&loop, UV_RUN_DEFAULT);
  EXPECT_EQ(uv_loop_close(&loop), 0);
}

TEST(TimerWatcherTest, StartStop) {
  uv_loop_t loop;
  EXPECT_EQ(uv_loop_init(&loop), 0);
  uv_async_t checkLocal;
  uv_async_t checkRemote;
  const auto interval = 10;
  int counter = 0;
  EXPECT_EQ(uv_async_init(&loop, &checkLocal,
                          [](uv_async_t *handle) {
                            auto counter_ptr =
                                reinterpret_cast<int *>(handle->data);
                            (*counter_ptr)++;
                          }),
            0);
  checkLocal.data = &counter;
  EXPECT_EQ(uv_async_init(&loop, &checkRemote,
                          [](uv_async_t *handle) {
                            // Do nothing for remote check at this moment
                          }),
            0);
  {
    auto watcher = new TimerWatcher(&checkLocal, &checkRemote, &loop, interval);
    uv_timer_t stopTimer;
    uv_timer_init(&loop, &stopTimer);
    watcher->startWatch();
    struct TimerData {
      TimerWatcher *watcher;
      uv_timer_t *stopTimer;
      uv_async_t *checkLocal;
      uv_async_t *checkRemote;
      uv_loop_t *loop;
    };
    TimerData timerData{watcher, &stopTimer, &checkLocal, &checkRemote, &loop};
    stopTimer.data = &timerData;

    // Starting a timer to stop the watcher
    uv_timer_start(
        &stopTimer,
        [](uv_timer_t *handle) {
          TimerData *timerData = reinterpret_cast<TimerData *>(handle->data);
          TimerWatcher *watcher = timerData->watcher;
          watcher->stopWatch();
          delete watcher;

          uv_close(reinterpret_cast<uv_handle_t *>(timerData->checkLocal),
                   nullptr);
          uv_close(reinterpret_cast<uv_handle_t *>(timerData->checkRemote),
                   nullptr);
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
