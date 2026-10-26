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
  auto l = new uv_loop_t();
  auto result = uv_loop_init(l);
  EXPECT_EQ(result, 0);
  auto loop = LoopPtr(l, deleteLoop);

  corral::run(*loop, [&loop]() -> corral::Task<void> {
    CORRAL_WITH_NURSERY(n) {
      auto process = new HgProcess(*loop, n);
      co_await process->shutdown();
      delete process;
      co_return corral::join;
    };
  });
}

TEST_F(HgProcessTest, NonExistentFolderSummary) {
  auto l = new uv_loop_t();
  auto result = uv_loop_init(l);
  EXPECT_EQ(result, 0);
  auto loop = LoopPtr(l, deleteLoop);
  bool taskExecuted = false;

  corral::run(*loop, [&loop, &taskExecuted]() -> corral::Task<void> {
    CORRAL_WITH_NURSERY(n) {
      auto process = new HgProcess(*loop, n);
      auto result = co_await process->runCommand(
          {"summary", "--repository", "non-existent-folder"});
      taskExecuted = true;
      EXPECT_EQ(result.resultCode, 255);
      EXPECT_FALSE(result.requiresInput);
      EXPECT_EQ(result.requiredInputSize, 0);
      co_await process->shutdown();
      delete process;
      co_return corral::join;
    };
  });
  EXPECT_TRUE(taskExecuted);
}

TEST_F(HgProcessTest, RunCommandHelp) {
  auto l = new uv_loop_t();
  auto result = uv_loop_init(l);
  EXPECT_EQ(result, 0);
  auto loop = LoopPtr(l, deleteLoop);
  bool taskExecuted = false;

  corral::run(*loop, [&loop, &taskExecuted]() -> corral::Task<void> {
    CORRAL_WITH_NURSERY(n) {
      auto process = new HgProcess(*loop, n);
      auto result = co_await process->runCommand({"help"});
      taskExecuted = true;
      EXPECT_EQ(result.resultCode, 0);
      EXPECT_EQ(result.output.find("Mercurial Distributed SCM"), 0);
      EXPECT_FALSE(result.requiresInput);
      EXPECT_EQ(result.requiredInputSize, 0);
      co_await process->shutdown();
      delete process;
      co_return corral::join;
    };
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
  bool taskExecuted = false;

  corral::run(*loop, [&loop, &taskExecuted]() -> corral::Task<void> {
    CORRAL_WITH_NURSERY(n) {
      auto process = new HgProcess(*loop, n);
      auto tempRepoPath = createTemporaryFolder("temp_repo");

      // Now init repo
      auto result =
          co_await process->runCommand({"init", tempRepoPath.string()});
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
      co_return corral::join;
    };
  });
  EXPECT_TRUE(taskExecuted);
}
