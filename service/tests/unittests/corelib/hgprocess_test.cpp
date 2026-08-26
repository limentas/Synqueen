#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "corelib/src/patch/hgprocess.hpp"
#include "utils/corraleventlooptraits.hpp"
#include "utils/corralutils.hpp"
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

TEST(HgProcessTest, NonExistentFolderSummary) {
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
    EXPECT_FALSE(result.requiresInput);
    EXPECT_EQ(result.requiredInputSize, 0);
    co_await process->shutdown();
    delete process;
  });
  EXPECT_TRUE(taskExecuted);
}

TEST(HgProcessTest, RunCommandHelp) {
  auto l = new uv_loop_t();
  auto result = uv_loop_init(l);
  EXPECT_EQ(result, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto process = new HgProcess(loop.get());
  bool taskExecuted = false;

  corral::run(*loop, [process, &taskExecuted]() -> corral::Task<void> {
    auto result = co_await process->runCommand({"help"});
    taskExecuted = true;
    EXPECT_EQ(result.resultCode, 0);
    EXPECT_EQ(result.output.find("Mercurial Distributed SCM"), 0);
    EXPECT_FALSE(result.requiresInput);
    EXPECT_EQ(result.requiredInputSize, 0);
    co_await process->shutdown();
    delete process;
  });
  EXPECT_TRUE(taskExecuted);
}
