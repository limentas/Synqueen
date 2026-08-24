#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "utils/uvutils.hpp"

using namespace testing;
using namespace std;
using namespace synqueen;

TEST(UvUtilsTest, DeleteSignal) {
  uv_loop_t loop;
  EXPECT_EQ(uv_loop_init(&loop), 0);
  {
    // Signal lifetime should be shorter than loop lifetime
    auto signal = SignalPtr(new uv_signal_t(), deleteSignal);
    EXPECT_EQ(uv_signal_init(&loop, signal.get()), 0);
  }
  uv_run(&loop, UV_RUN_DEFAULT);
  EXPECT_EQ(uv_loop_close(&loop), 0);
}

TEST(UvUtilsTest, DeleteLoop) {
  LoopPtr loopPtr(new uv_loop_t(), deleteLoop);
  EXPECT_EQ(uv_loop_init(loopPtr.get()), 0);
  uv_run(loopPtr.get(), UV_RUN_DEFAULT);
}

// This test is to check that deleteLoop() can handle the case where there are
// still active handles in the loop.
TEST(UvUtilsTest, DeleteLoopDanglingHandle) {
  LoopPtr loopPtr(new uv_loop_t(), deleteLoop);
  EXPECT_EQ(uv_loop_init(loopPtr.get()), 0);

  auto async = AsyncPtr(new uv_async_t(), deleteHandle<uv_async_t>);
  EXPECT_EQ(
      uv_async_init(loopPtr.get(), async.get(), [](uv_async_t *handle) {}), 0);
}

TEST(UvUtilsTest, DeleteAsync) {
  uv_loop_t loop;
  EXPECT_EQ(uv_loop_init(&loop), 0);
  {
    auto async = AsyncPtr(new uv_async_t(), deleteHandle<uv_async_t>);
    EXPECT_EQ(uv_async_init(&loop, async.get(), [](uv_async_t *handle) {}), 0);
  }
  uv_run(&loop, UV_RUN_DEFAULT);
  EXPECT_EQ(uv_loop_close(&loop), 0);
}
