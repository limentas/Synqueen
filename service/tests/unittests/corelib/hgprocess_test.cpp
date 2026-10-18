#include "corelib/src/patch/hgprocess.hpp"
#include "testutils.hpp"
#include "utils/corraleventlooptraits.hpp"
#include "utils/corralutils.hpp"
#include "utils/logger.hpp"
#include "utils/standardpaths.hpp"
#include "utils/uvutils.hpp"

#include <filesystem>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

using namespace testing;
using namespace std;
using namespace std::string_literals;
using namespace synqueen;
using namespace synqueen::patch;

class HgProcessTest : public testing::Test {
protected:
  void SetUp() override { StandardPaths::initializeInTemp("Synqueen-test"); }
  void TearDown() override { cleanupAppFolders(); }
};

TEST_F(HgProcessTest, CtorDtor) {
  uv_loop_t loop;
  EXPECT_EQ(uv_loop_init(&loop), 0);
  HgProcess process(loop);
  uv_run(&loop, UV_RUN_DEFAULT);
  EXPECT_EQ(uv_loop_close(&loop), 0);
}

TEST_F(HgProcessTest, NonExistentFolderSummary) {
  auto l = new uv_loop_t();
  auto result = uv_loop_init(l);
  EXPECT_EQ(result, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto process = new HgProcess(*loop);
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

TEST_F(HgProcessTest, RunCommandHelp) {
  auto l = new uv_loop_t();
  auto result = uv_loop_init(l);
  EXPECT_EQ(result, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto process = new HgProcess(*loop);
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

TEST_F(HgProcessTest, NormalRepoSummary) {
  // Let's create a repo in a temporary folder and then run the summary command
  // on it.
  auto l = new uv_loop_t();
  auto result = uv_loop_init(l);
  EXPECT_EQ(result, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto process = new HgProcess(*loop);
  bool taskExecuted = false;

  corral::run(*loop, [&loop, process, &taskExecuted]() -> corral::Task<void> {
    auto tempRepoPath = createTemporaryFolder("temp_repo");

    // Now init repo
    auto result = co_await process->runCommand({"init", tempRepoPath.string()});
    [&result]() {
      ASSERT_EQ(result.resultCode, 0) << "Failed to init repo";
      EXPECT_FALSE(result.requiresInput);
      EXPECT_EQ(result.requiredInputSize, 0);
      EXPECT_THAT(result.error, IsEmpty());
    }();

    // Run summary
    result = co_await process->runCommand(
        {"summary", "--repository", tempRepoPath.string()});
    [&result]() {
      ASSERT_EQ(result.resultCode, 0) << "Failed to run summary";
      EXPECT_FALSE(result.requiresInput);
      EXPECT_EQ(result.requiredInputSize, 0);
      EXPECT_THAT(result.error, IsEmpty());
    }();

    taskExecuted = true;
    co_await process->shutdown();
    delete process;

    // Cleanup temporary folder
    std::error_code ec;
    std::filesystem::remove_all(tempRepoPath, ec);
    if (ec) {
      SPDLOG_ERROR("Failed to remove temporary repo folder: {}. Error: {}",
                   tempRepoPath, ec.message());
    }
  });
  EXPECT_TRUE(taskExecuted);
}
