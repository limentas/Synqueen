#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "corelib/src/patch/hgprocess.hpp"
#include "utils/uvutils.hpp"

using namespace testing;
using namespace std;
using namespace std::string_literals;
using namespace synqueen;

TEST(HgProcessTest, CtorDtor) {
  uv_loop_t loop;
  EXPECT_EQ(uv_loop_init(&loop), 0);
  HgProcess process(&loop);
  uv_run(&loop, UV_RUN_DEFAULT);
  EXPECT_EQ(uv_loop_close(&loop), 0);
}

namespace corral {
template <> struct EventLoopTraits<uv_loop_t> {
  static EventLoopID eventLoopID(uv_loop_t &app) { return EventLoopID(&app); }
  static void run(uv_loop_t &loop) { uv_run(&loop, UV_RUN_DEFAULT); }
  static void stop(uv_loop_t &loop) {
    // The loop should close itself when there are no more active handles
    // uv_stop(&loop);
  }
};
} // namespace corral

TEST(HgProcessTest, RunCommandWithNonExistentFolder) {
  auto l = new uv_loop_t();
  auto result = uv_loop_init(l);
  EXPECT_EQ(result, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto process = new HgProcess(loop.get());
  bool taskExecuted = false;

  corral::run(*loop, [process, &taskExecuted]() -> corral::Task<void> {
    auto result = co_await process->runCommand(
        {"summary", "--repository", "non-existent-folder"});
    taskExecuted = true;
    EXPECT_EQ(result.resultCode, 255);
    co_await process->shutdown();
    delete process;
  });
  EXPECT_TRUE(taskExecuted);
}
