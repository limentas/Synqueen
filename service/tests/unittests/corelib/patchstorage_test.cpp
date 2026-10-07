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

TEST(PatchStorageTest, MovePatchToApply) {
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
      storage.movePatchToApply(patchFile);
      const auto &patches2 = storage.listPatches();
      EXPECT_EQ(incomingNotificationsCount, 1);
      EXPECT_EQ(outgoingNotificationsCount, 0);
      EXPECT_THAT(patches2, SizeIs(1));
      EXPECT_EQ(patches2.at(1).index, 1);
      EXPECT_EQ(patches2.at(1).lastCommit, "ccaabbeeffff");
      EXPECT_THAT(patches2.at(1).path.string(), StartsWith(tempFolder));
      EXPECT_EQ(patches2.at(1).kind, PatchStorage::PatchKind::ToApply);

      // Create another temp patch file
      auto patchFile2 = fs::path(tempFolder) / "22_ddaabbeeffff.patch";
      std::ofstream(patchFile2) << "patch content2";

      storage.movePatchToApply(patchFile2);
      const auto &patches3 = storage.listPatches();
      EXPECT_EQ(incomingNotificationsCount, 2);
      EXPECT_EQ(outgoingNotificationsCount, 0);
      EXPECT_THAT(patches3, SizeIs(2));
      EXPECT_EQ(patches3.at(22).index, 22);
      EXPECT_EQ(patches3.at(22).lastCommit, "ddaabbeeffff");
      EXPECT_THAT(patches3.at(22).path.string(), StartsWith(tempFolder));
      EXPECT_EQ(patches3.at(22).kind, PatchStorage::PatchKind::ToApply);
    }

    removeFileOrFolder(tempFolder);
    co_return;
  });
}

TEST(PatchStorageTest, StagePatchAsApplied) {
  // TODO
}

TEST(PatchStorageTest, MovePatchToOutgoing) {
  // TODO
}
