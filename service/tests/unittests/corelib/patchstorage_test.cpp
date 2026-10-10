#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "patchstorage.hpp"
#include "testutils.hpp"
#include "utils/corraleventlooptraits.hpp"
#include "utils/corralheader.hpp"
#include "utils/utils.hpp"
#include "utils/uvutils.hpp"

using namespace testing;
using namespace std;
using namespace std::string_literals;
using namespace synqueen;
using namespace synqueen::utils;

namespace fs = std::filesystem;

TEST(PatchStorageTest, CtorDtor) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    auto tempFolder =
        co_await createTemporaryFolder("sq_test_XXXXXX", loop.get());

    {
      PatchStorage storage(tempFolder);
    }

    removeFileOrFolder(tempFolder);
    co_return;
  });
}

TEST(PatchStorageTest, MovePatchesToApply) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    auto tempFolder =
        co_await createTemporaryFolder("sq_test_XXXXXX", loop.get());

    {
      PatchStorage storage(tempFolder);

      const auto &patches1 = storage.listPatches();
      EXPECT_THAT(patches1, IsEmpty());

      // Create a temp patch file
      auto patchFile1 = fs::path(tempFolder) / "1_ccaabbeeffff.patch";
      auto patchFile2 = fs::path(tempFolder) / "10_aabbeedff.patch";
      auto patchFile3 = fs::path(tempFolder) / "22_ddaabbeeffff.patch";
      std::ofstream(patchFile1) << "patch content";
      std::ofstream(patchFile2) << "patch content2";
      std::ofstream(patchFile3) << "patch content3";

      auto incomingNotificationsCount = 0, outgoingNotificationsCount = 0;
      storage.getIncomingNotifier().addSyncHandler(
          [&incomingNotificationsCount]() { incomingNotificationsCount++; });
      storage.getOutgoingNotifier().addSyncHandler(
          [&outgoingNotificationsCount]() { outgoingNotificationsCount++; });
      storage.movePatchesToApply({patchFile1, patchFile2, patchFile3});
      const auto &patches2 = storage.listPatches();
      EXPECT_EQ(incomingNotificationsCount, 1);
      EXPECT_EQ(outgoingNotificationsCount, 0);
      EXPECT_THAT(patches2, SizeIs(3));
      EXPECT_EQ(patches2.at(1).index, 1);
      EXPECT_EQ(patches2.at(1).lastCommit, "ccaabbeeffff");
      EXPECT_THAT(patches2.at(1).path.string(), StartsWith(tempFolder));
      EXPECT_EQ(patches2.at(1).kind, PatchStorage::PatchKind::ToApply);
      EXPECT_EQ(patches2.at(10).index, 10);
      EXPECT_EQ(patches2.at(10).lastCommit, "aabbeedff");
      EXPECT_THAT(patches2.at(10).path.string(), StartsWith(tempFolder));
      EXPECT_EQ(patches2.at(10).kind, PatchStorage::PatchKind::ToApply);
      EXPECT_EQ(patches2.at(22).index, 22);
      EXPECT_EQ(patches2.at(22).lastCommit, "ddaabbeeffff");
      EXPECT_THAT(patches2.at(22).path.string(), StartsWith(tempFolder));
      EXPECT_EQ(patches2.at(22).kind, PatchStorage::PatchKind::ToApply);

      // Create another temp patch file
      auto patchFile4 = fs::path(tempFolder) / "15_ffeadbcefd.patch";
      std::ofstream(patchFile4) << "patch content4";

      storage.movePatchesToApply({patchFile4});
      const auto &patches3 = storage.listPatches();
      EXPECT_EQ(incomingNotificationsCount, 2);
      EXPECT_EQ(outgoingNotificationsCount, 0);
      EXPECT_THAT(patches3, SizeIs(4));
      EXPECT_EQ(patches3.at(15).index, 15);
      EXPECT_EQ(patches3.at(15).lastCommit, "ffeadbcefd");
      EXPECT_THAT(patches3.at(15).path.string(), StartsWith(tempFolder));
      EXPECT_EQ(patches3.at(15).kind, PatchStorage::PatchKind::ToApply);
    }

    removeFileOrFolder(tempFolder);
    co_return;
  });
}

TEST(PatchStorageTest, StagePatchAsApplied) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);

  corral::run(*loop, [&loop]() -> corral::Task<void> {
    auto tempFolder =
        co_await createTemporaryFolder("sq_test_XXXXXX", loop.get());

    {
      PatchStorage storage(tempFolder);

      const auto &patches1 = storage.listPatches();
      EXPECT_THAT(patches1, IsEmpty());

      // Create a temp patch file
      auto patchFile = fs::path(tempFolder) / "1_ccaabbeeffff.patch";
      std::ofstream(patchFile) << "patch content";

      auto incomingNotificationsCount = 0, outgoingNotificationsCount = 0;
      storage.getIncomingNotifier().addSyncHandler(
          [&incomingNotificationsCount]() { incomingNotificationsCount++; });
      storage.getOutgoingNotifier().addSyncHandler(
          [&outgoingNotificationsCount]() { outgoingNotificationsCount++; });
      storage.stagePatchAsApplied(patchFile);
      const auto &patches2 = storage.listPatches();
      EXPECT_EQ(incomingNotificationsCount, 1);
      EXPECT_EQ(outgoingNotificationsCount, 0);
      EXPECT_THAT(patches2, SizeIs(1));
      EXPECT_EQ(patches2.at(1).index, 1);
      EXPECT_EQ(patches2.at(1).lastCommit, "ccaabbeeffff");
      EXPECT_THAT(patches2.at(1).path.string(), StartsWith(tempFolder));
      EXPECT_EQ(patches2.at(1).kind, PatchStorage::PatchKind::Applied);
    }

    removeFileOrFolder(tempFolder);
    co_return;
  });
}

TEST(PatchStorageTest, MovePatchToOutgoing) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);

  corral::run(*loop, [&loop]() -> corral::Task<void> {
    auto tempFolder =
        co_await createTemporaryFolder("sq_test_XXXXXX", loop.get());

    {
      PatchStorage storage(tempFolder);

      const auto &patches1 = storage.listPatches();
      EXPECT_THAT(patches1, IsEmpty());

      // Create a temp patch file
      auto patchFile = fs::path(tempFolder) / "1_ccaabbeeffff.patch";
      std::ofstream(patchFile) << "patch content";

      auto incomingNotificationsCount = 0, outgoingNotificationsCount = 0;
      storage.getIncomingNotifier().addSyncHandler(
          [&incomingNotificationsCount]() { incomingNotificationsCount++; });
      storage.getOutgoingNotifier().addSyncHandler(
          [&outgoingNotificationsCount]() { outgoingNotificationsCount++; });
      storage.movePatchToOutgoing(patchFile);
      const auto &patches2 = storage.listPatches();
      EXPECT_EQ(incomingNotificationsCount, 0);
      EXPECT_EQ(outgoingNotificationsCount, 1);
      EXPECT_THAT(patches2, SizeIs(1));
      EXPECT_EQ(patches2.at(1).index, 1);
      EXPECT_EQ(patches2.at(1).lastCommit, "ccaabbeeffff");
      EXPECT_THAT(patches2.at(1).path.string(), StartsWith(tempFolder));
      EXPECT_EQ(patches2.at(1).kind, PatchStorage::PatchKind::Outgoing);
    }

    removeFileOrFolder(tempFolder);
    co_return;
  });
}
