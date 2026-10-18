#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "patchstorage.hpp"
#include "testutils.hpp"
#include "utils/corraleventlooptraits.hpp"
#include "utils/corralheader.hpp"
#include "utils/standardpaths.hpp"
#include "utils/utils.hpp"
#include "utils/uvutils.hpp"

using namespace testing;
using namespace std;
using namespace std::string_literals;
using namespace synqueen;
using namespace synqueen::utils;

namespace fs = std::filesystem;

class PatchStorageTest : public testing::Test {
protected:
  void SetUp() override { StandardPaths::initializeInTemp("Synqueen-test"); }
  void TearDown() override { cleanupAppFolders(); }
};

TEST_F(PatchStorageTest, CtorDtor) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    auto tempFolder = createTemporaryFolder("sq_test");
    PatchStorage storage(tempFolder);
    co_return;
  });
}

TEST_F(PatchStorageTest, MovePatchesToApply) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    auto tempFolder = createTemporaryFolder("sq_test");
    PatchStorage storage(tempFolder);

    EXPECT_THAT(storage.listPatchesToApply(), IsEmpty());
    EXPECT_THAT(storage.listAppliedPatches(), IsEmpty());
    EXPECT_THAT(storage.listOutgoingPatches(), IsEmpty());

    // Create a temp patch file
    const auto patchFileName1 = "1_ccaabbeeffff.patch";
    const auto patchFileName2 = "10_aabbeedff.patch";
    const auto patchFileName3 = "22_ddaabbeeffff.patch";
    const auto patchFile1 = tempFolder / patchFileName1;
    const auto patchFile2 = tempFolder / patchFileName2;
    const auto patchFile3 = tempFolder / patchFileName3;
    std::ofstream(patchFile1) << "patch content";
    std::ofstream(patchFile2) << "patch content2";
    std::ofstream(patchFile3) << "patch content3";

    auto incomingNotificationsCount = 0, outgoingNotificationsCount = 0;
    storage.getIncomingNotifier().addSyncHandler(
        [&incomingNotificationsCount]() { incomingNotificationsCount++; });
    storage.getOutgoingNotifier().addSyncHandler(
        [&outgoingNotificationsCount]() { outgoingNotificationsCount++; });
    storage.movePatchesToApply({patchFile1, patchFile2, patchFile3});
    EXPECT_THAT(storage.listAppliedPatches(), IsEmpty());
    EXPECT_THAT(storage.listOutgoingPatches(), IsEmpty());
    const auto &patches1 = storage.listPatchesToApply();
    EXPECT_EQ(incomingNotificationsCount, 1);
    EXPECT_EQ(outgoingNotificationsCount, 0);
    EXPECT_THAT(patches1, SizeIs(3));
    EXPECT_THAT(patches1.find(patchFileName1)->second.string(),
                StartsWith(tempFolder.string()));
    EXPECT_THAT(patches1.find(patchFileName2)->second.string(),
                StartsWith(tempFolder.string()));
    EXPECT_THAT(patches1.find(patchFileName3)->second.string(),
                StartsWith(tempFolder.string()));

    // Create another temp patch file
    const auto patchFileName4 = "15_ffeadbcefd.patch";
    const auto patchFile4 = tempFolder / patchFileName4;
    std::ofstream(patchFile4) << "patch content4";

    storage.movePatchesToApply({patchFile4});
    EXPECT_THAT(storage.listAppliedPatches(), IsEmpty());
    EXPECT_THAT(storage.listOutgoingPatches(), IsEmpty());
    const auto &patches2 = storage.listPatchesToApply();
    EXPECT_EQ(incomingNotificationsCount, 2);
    EXPECT_EQ(outgoingNotificationsCount, 0);
    EXPECT_THAT(patches2, SizeIs(4));
    EXPECT_THAT(patches2.find(patchFileName4)->second.string(),
                StartsWith(tempFolder.string()));
    co_return;
  });
}

TEST_F(PatchStorageTest, StagePatchAsApplied) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);

  corral::run(*loop, [&loop]() -> corral::Task<void> {
    auto tempFolder = createTemporaryFolder("sq_test");
    PatchStorage storage(tempFolder);

    EXPECT_THAT(storage.listPatchesToApply(), IsEmpty());
    EXPECT_THAT(storage.listAppliedPatches(), IsEmpty());
    EXPECT_THAT(storage.listOutgoingPatches(), IsEmpty());

    // Create a temp patch file
    const auto patchFileName = "1_ccaabbeeffff.patch";
    auto patchFile = tempFolder / patchFileName;
    std::ofstream(patchFile) << "patch content";

    auto incomingNotificationsCount = 0, outgoingNotificationsCount = 0;
    storage.getIncomingNotifier().addSyncHandler(
        [&incomingNotificationsCount]() { incomingNotificationsCount++; });
    storage.getOutgoingNotifier().addSyncHandler(
        [&outgoingNotificationsCount]() { outgoingNotificationsCount++; });
    storage.stagePatchAsApplied(patchFile);
    EXPECT_THAT(storage.listPatchesToApply(), IsEmpty());
    EXPECT_THAT(storage.listOutgoingPatches(), IsEmpty());
    // Moving a patch to applied should not trigger incoming notifications
    EXPECT_EQ(incomingNotificationsCount, 0);
    EXPECT_EQ(outgoingNotificationsCount, 0);

    const auto &patches1 = storage.listAppliedPatches();
    EXPECT_THAT(patches1, SizeIs(1));
    EXPECT_THAT(patches1.find(patchFileName)->second.string(),
                StartsWith(tempFolder.string()));
    co_return;
  });
}

TEST_F(PatchStorageTest, MovePatchToOutgoing) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);

  corral::run(*loop, [&loop]() -> corral::Task<void> {
    auto tempFolder = createTemporaryFolder("sq_test");
    PatchStorage storage(tempFolder);

    EXPECT_THAT(storage.listPatchesToApply(), IsEmpty());
    EXPECT_THAT(storage.listAppliedPatches(), IsEmpty());
    EXPECT_THAT(storage.listOutgoingPatches(), IsEmpty());

    // Create a temp patch file
    const auto patchFileName = "temp.patch";
    const auto patchFile = tempFolder / patchFileName;
    std::ofstream(patchFile) << "patch content";

    auto incomingNotificationsCount = 0, outgoingNotificationsCount = 0;
    storage.getIncomingNotifier().addSyncHandler(
        [&incomingNotificationsCount]() { incomingNotificationsCount++; });
    storage.getOutgoingNotifier().addSyncHandler(
        [&outgoingNotificationsCount]() { outgoingNotificationsCount++; });
    storage.movePatchToOutgoing(patchFile, 1, "ccaabbeeffff");
    EXPECT_THAT(storage.listPatchesToApply(), IsEmpty());
    EXPECT_THAT(storage.listAppliedPatches(), IsEmpty());
    EXPECT_EQ(incomingNotificationsCount, 0);
    EXPECT_EQ(outgoingNotificationsCount, 1);

    const auto &patches1 = storage.listOutgoingPatches();
    EXPECT_THAT(patches1, SizeIs(1));
    EXPECT_EQ(patches1.find("1_ccaabbeeffff.patch")->second.index, 1);
    EXPECT_EQ(patches1.find("1_ccaabbeeffff.patch")->second.lastCommit,
              "ccaabbeeffff");
    EXPECT_THAT(patches1.find("1_ccaabbeeffff.patch")->second.path.string(),
                StartsWith(tempFolder.string()));
    co_return;
  });
}
