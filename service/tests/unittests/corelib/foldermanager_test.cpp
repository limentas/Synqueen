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
  MOCK_METHOD(corral::Task<ApplyPatchResult>, applyPatches,
              (const std::filesystem::path &,
               const std::list<std::filesystem::path> &),
              (override));
};

class MockPatchStorage : public IPatchStorage {
public:
  MOCK_METHOD(void, movePatchesToApply,
              (const std::list<std::filesystem::path> &), (override));
  MOCK_METHOD(void, stagePatchAsApplied, (const std::filesystem::path &),
              (override));
  MOCK_METHOD(void, movePatchToOutgoing,
              (const std::filesystem::path &, int, const std::string &),
              (override));
  MOCK_METHOD((const PatchesByFileName &), listOutgoingPatches, (),
              (const, override));
  MOCK_METHOD((const PatchFilesByFileName &), listPatchesToApply, (),
              (const, override));
  MOCK_METHOD((const PatchFilesByFileName &), listAppliedPatches, (),
              (const, override));
  MOCK_METHOD(utils::ISubscribable &, getIncomingNotifier, (), (override));
  MOCK_METHOD(utils::ISubscribable &, getOutgoingNotifier, (), (override));
};

class FolderManagerTest : public testing::Test {
protected:
  void SetUp() override { StandardPaths::initializeInTemp("Synqueen-test"); }
  void TearDown() override { cleanupAppFolders(); }
};

TEST_F(FolderManagerTest, CtorDtor) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  MockPatchProvider patchProvider;
  MockPatchStorage patchStorage;
  corral::run(*loop,
              [&patchProvider, &patchStorage, &loop]() -> corral::Task<void> {
                auto tempRepoPath = createTemporaryFolder("temp_out");
                FolderManager folderManager("1", tempRepoPath, patchProvider,
                                            patchStorage);
                co_return;
              });
}

TEST_F(FolderManagerTest, LoadState) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  MockPatchProvider patchProvider;
  corral::run(*loop, [&patchProvider, &loop]() -> corral::Task<void> {
    auto tempRepoPath = createTemporaryFolder("temp_out");

    // To test loading state we need at first create it
    {
      MockPatchStorage patchStorage;
      FolderManager folderManager("1", tempRepoPath, patchProvider,
                                  patchStorage);
      CORRAL_WITH_NURSERY(n) {
        IPatchStorage::PatchesByFileName emptyPatches;
        Notifier incomingNotifier;
        // TODO:
        EXPECT_CALL(patchStorage, listOutgoingPatches())
            .Times(1)
            .WillOnce(testing::ReturnRef(emptyPatches));
        EXPECT_CALL(patchStorage, getIncomingNotifier())
            .Times(1)
            .WillOnce(testing::ReturnRef(incomingNotifier));
        folderManager.initialize(n);
        co_return corral::join;
      };
    }
  });
}

// TODO: make this test data driven and figure out which mock calls should be
// uninterested and which should have expectations
TEST_F(FolderManagerTest, Synchronize) {
  GTEST_SKIP()
      << "The test is not well done. Needs proper mock data and expectations.";
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  MockPatchProvider patchProvider;
  MockPatchStorage patchStorage;
  corral::run(
      *loop, [&patchProvider, &patchStorage, &loop]() -> corral::Task<void> {
        auto tempRepoPath = createTemporaryFolder("temp_out");
        FolderManager folderManager("1", tempRepoPath, patchProvider,
                                    patchStorage);
        CORRAL_WITH_NURSERY(n) {
          IPatchStorage::PatchesByFileName emptyPatches;
          Notifier incomingNotifier;
          EXPECT_CALL(patchStorage, listOutgoingPatches())
              .Times(1)
              .WillOnce(testing::ReturnRef(emptyPatches));
          EXPECT_CALL(patchStorage, getIncomingNotifier())
              .Times(1)
              .WillOnce(testing::ReturnRef(incomingNotifier));
          folderManager.initialize(n);

          TestNotifiableWaiter waiter(*loop);
          Sequence s;
          // No changes
          EXPECT_CALL(patchProvider, checkLocalState(tempRepoPath))
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
          EXPECT_CALL(patchStorage, listPatchesToApply())
              .Times(1)
              .InSequence(s)
              .WillOnce([]() -> const IPatchStorage::PatchFilesByFileName & {
                static const IPatchStorage::PatchFilesByFileName emptyMap{};
                return emptyMap;
              });
          folderManager.synchronize();
          EXPECT_TRUE(co_await waiter.waitFor(500ms));

          // Now let's simulate a change this will trigger patch preparation
          waiter.reset();
          EXPECT_CALL(patchProvider, checkLocalState(tempRepoPath))
              .Times(1)
              .InSequence(s)
              .WillOnce([]() -> corral::Task<LocalStateResult> {
                return corral::just(
                    LocalStateResult{.initialized = true,
                                     .hasUncommittedChanges = true,
                                     .hasConflicts = false,
                                     .lastCommitHash = "hash1"});
              });
          EXPECT_CALL(patchProvider, preparePatch(tempRepoPath, std::string()))
              .Times(1)
              .InSequence(s)
              .WillOnce([&waiter,
                         &tempRepoPath]() -> corral::Task<PreparePatchResult> {
                // Create patch file
                auto patchPath = tempRepoPath / "patch1";
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
      });
}
