#include "corelib/src/foldermanager.hpp"
#include "testutils.hpp"
#include "utils/corraleventlooptraits.hpp"
#include "utils/corralheader.hpp"
#include "utils/utils.hpp"
#include "utils/uvutils.hpp"

#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using ::testing::Sequence;
using namespace std;
using namespace std::string_literals;
using namespace synqueen;
using namespace synqueen::patch;
using namespace synqueen::utils;
using namespace std::chrono_literals;

namespace fs = std::filesystem;

class MockPatchProvider : public IPatchProvider {
public:
  MOCK_METHOD(corral::Task<LocalStateResult>, checkLocalState,
              (const std::filesystem::path &), (override));
  MOCK_METHOD(corral::Task<PreparePatchResult>, preparePatch,
              (const std::filesystem::path &, const std::string &), (override));
};

class MockPatchStorage : public IPatchStorage {
public:
  MOCK_METHOD(void, movePatchesToApply,
              (const std::list<std::filesystem::path> &), (override));
  MOCK_METHOD(void, stagePatchAsApplied, (const std::filesystem::path &),
              (override));
  MOCK_METHOD(void, movePatchToOutgoing, (const std::filesystem::path &),
              (override));
  MOCK_METHOD((const std::map<int, IPatchStorage::PatchFile> &), listPatches,
              (), (const, override));
  MOCK_METHOD(utils::ISubscribable &, getIncomingNotifier, (), (override));
  MOCK_METHOD(utils::ISubscribable &, getOutgoingNotifier, (), (override));
};

TEST(FolderManagerTest, CtorDtor) {
  StandardPaths::initialize("Synqueen-test");
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  MockPatchProvider patchProvider;
  MockPatchStorage patchStorage;
  corral::run(
      *loop, [&patchProvider, &patchStorage, &loop]() -> corral::Task<void> {
        auto tempRepoPath =
            co_await createTemporaryFolder("temp_out_XXXXXX", loop.get());

        {
          FolderManager folderManager("1", tempRepoPath, patchProvider,
                                      patchStorage);
        }

        removeFileOrFolder(tempRepoPath);
      });

  // Cleanup the data folder after the test
  removeFileOrFolder(StandardPaths::getDataPath().string());
}

TEST(FolderManagerTest, LoadState) {
  StandardPaths::initialize("Synqueen-test");
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  MockPatchProvider patchProvider;
  corral::run(*loop, [&patchProvider, &loop]() -> corral::Task<void> {
    auto tempRepoPath =
        co_await createTemporaryFolder("temp_out_XXXXXX", loop.get());

    // To test loading state we need at first create it
    {
      MockPatchStorage patchStorage;
      FolderManager folderManager("1", tempRepoPath, patchProvider,
                                  patchStorage);
      CORRAL_WITH_NURSERY(n) {
        folderManager.setupState(n);
        co_return corral::join;
      };
    }

    // Now let's load state
    {
      MockPatchStorage patchStorage;
      FolderManager folderManager("1", tempRepoPath, patchProvider,
                                  patchStorage);
      CORRAL_WITH_NURSERY(n) {
        folderManager.loadState(n);
        co_return corral::join;
      };
    }

    removeFileOrFolder(tempRepoPath);
  });

  // Cleanup the data folder after the test
  removeFileOrFolder(StandardPaths::getDataPath().string());
}

TEST(FolderManagerTest, CheckState) {
  StandardPaths::initialize("Synqueen-test");
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  MockPatchProvider patchProvider;
  MockPatchStorage patchStorage;
  corral::run(
      *loop, [&patchProvider, &patchStorage, &loop]() -> corral::Task<void> {
        auto tempRepoPath =
            co_await createTemporaryFolder("temp_out_XXXXXX", loop.get());

        {
          FolderManager folderManager("1", tempRepoPath, patchProvider,
                                      patchStorage);
          CORRAL_WITH_NURSERY(n) {
            folderManager.setupState(n);

            TestWaiter waiter(loop.get());
            Sequence s;
            // No changes
            EXPECT_CALL(patchProvider, checkLocalState(fs::path(tempRepoPath)))
                .Times(1)
                .InSequence(s)
                .WillOnce([&waiter]() -> corral::Task<LocalStateResult> {
                  waiter.notify();
                  return corral::just(
                      LocalStateResult{.initialized = true,
                                       .hasUncommittedChanges = false,
                                       .hasConflicts = false,
                                       .lastCommitHash = "hash1"});
                });
            folderManager.synchronize();
            EXPECT_TRUE(co_await waiter.waitFor(500ms));

            // Now let's simulate a change this will trigger patch preparation
            waiter.reset();
            EXPECT_CALL(patchProvider, checkLocalState(fs::path(tempRepoPath)))
                .Times(1)
                .InSequence(s)
                .WillOnce([]() -> corral::Task<LocalStateResult> {
                  return corral::just(
                      LocalStateResult{.initialized = true,
                                       .hasUncommittedChanges = true,
                                       .hasConflicts = false,
                                       .lastCommitHash = "hash1"});
                });
            EXPECT_CALL(patchProvider,
                        preparePatch(fs::path(tempRepoPath), std::string()))
                .Times(1)
                .InSequence(s)
                .WillOnce([&waiter, &tempRepoPath]()
                              -> corral::Task<PreparePatchResult> {
                  // Create patch file
                  fs::path patchPath = fs::path(tempRepoPath) / "patch1";
                  std::ofstream(patchPath).put('a');
                  std::ofstream(patchPath).close();

                  waiter.notify();
                  return corral::just(
                      PreparePatchResult{.patchFilePath = patchPath,
                                         .lastIncludedCommitHash = "hash1"});
                });
            folderManager.synchronize();
            EXPECT_TRUE(co_await waiter.waitFor(500ms));

            co_return corral::join;
          };
        }

        removeFileOrFolder(tempRepoPath);
      });

  // Cleanup the data folder after the test
  removeFileOrFolder(StandardPaths::getDataPath().string());
}
