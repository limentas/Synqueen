#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "cloud/localfoldergate.hpp"
#include "synchronizer.hpp"
#include "testutils.hpp"
#include "utils/corraleventlooptraits.hpp"
#include "utils/corralheader.hpp"
#include "utils/standardpaths.hpp"

#include <filesystem>
#include <fstream>
#include <ranges>

using namespace testing;
using namespace synqueen;
using namespace synqueen::cloud;
using namespace synqueen::utils;
using namespace std::string_literals;
using namespace std::chrono_literals;

namespace fs = std::filesystem;

// Note: These tests are more integration-like because it tests the whole chain
// from filesystem to local cloud storage

class SynchronizerTest : public testing::Test {
protected:
  void SetUp() override { StandardPaths::initializeInTemp("Synqueen-test"); }
  void TearDown() override { cleanupAppFolders(); }
};

TEST_F(SynchronizerTest, CtorDtor) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    auto tempFolder = createTemporaryFolder("sq_test");
    Settings settings;
    Synchronizer synchronizer(*loop, settings, [](const Settings &s) {});
    co_return;
  });
}

TEST_F(SynchronizerTest, PatchUploadAndDownload) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    auto tempFolder = createTemporaryFolder("sq_test");
    auto cloudPath = tempFolder / "cloud";
    auto repoPath1 = tempFolder / "repo1";
    auto repoPath2 = tempFolder / "repo2";
    fs::create_directories(cloudPath);
    fs::create_directories(repoPath1);
    fs::create_directories(repoPath2);

    Settings settings{.useLocalCloud = true};
    Synchronizer synchronizer(*loop, settings, [](const Settings &s) {});
    CORRAL_WITH_NURSERY(n) {
      co_await n.start(
          [&synchronizer](corral::TaskStarted<> started) -> corral::Task<> {
            co_await synchronizer.run(std::move(started));
          });
      auto config = std::make_shared<LocalFolderConfig>();
      config->folderPath = cloudPath;
      co_await synchronizer.addFolder(IUiGateProvider::Folder{
          .path = repoPath1, .cloudDestinations = {config}});

      // Create a new file in the repo to trigger the upload
      std::ofstream(repoPath1 / "file.txt").put('a');
      synchronizer.checkAllLocal();

      TestConditionalWaiter waiter(
          *loop, [cloudPath]() { return filesInFolder(cloudPath) >= 1; });
      EXPECT_TRUE(co_await waiter.waitFor(500ms));
      EXPECT_EQ(filesInFolder(cloudPath), 1);

      // Change the file to trigger another upload
      std::ofstream(repoPath1 / "file.txt").put('b');
      synchronizer.checkAllLocal();

      TestConditionalWaiter waiter2(
          *loop, [cloudPath]() { return filesInFolder(cloudPath) >= 2; });
      EXPECT_TRUE(co_await waiter2.waitFor(500ms));
      EXPECT_EQ(filesInFolder(cloudPath), 2);

      // Now let's create another repo and point to the same local cloud path
      auto config2 = std::make_shared<LocalFolderConfig>();
      config2->folderPath = cloudPath;
      co_await synchronizer.addFolder(IUiGateProvider::Folder{
          .path = repoPath2, .cloudDestinations = {config2}});

      synchronizer.checkAllRemotes();

      TestConditionalWaiter waiter3(*loop, [repoPath1, repoPath2]() {
        return readAll(repoPath1 / "file.txt") ==
               readAll(repoPath2 / "file.txt");
      });
      EXPECT_TRUE(co_await waiter3.waitFor(500ms));
      // We need to give more time for rest commands to finish
      // (for example `hg summary`)
      co_await sleepFor(*loop, 200ms);

      // Now let's modify repo 2
      std::ofstream(repoPath2 / "file.txt").put('c');
      synchronizer.checkAllLocal();

      TestConditionalWaiter waiter4(
          *loop, [cloudPath]() { return filesInFolder(cloudPath) >= 3; });
      EXPECT_TRUE(co_await waiter4.waitFor(500ms));
      EXPECT_EQ(filesInFolder(cloudPath), 3);

      synchronizer.checkAllRemotes();
      TestConditionalWaiter waiter5(*loop, [repoPath1, repoPath2]() {
        return readAll(repoPath1 / "file.txt") ==
               readAll(repoPath2 / "file.txt");
      });
      EXPECT_TRUE(co_await waiter5.waitFor(500ms));

      // We need to give more time for rest commands to finish
      co_await sleepFor(*loop, 200ms);

      synchronizer.shutdown();
      co_return corral::join;
    };

    co_return;
  });
}

// Check how two folders sync via two same clouds
TEST_F(SynchronizerTest, TwoFoldersViaTwoClouds) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    auto tempFolder = createTemporaryFolder("sq_test");
    auto repoPath1 = tempFolder / "repo1";
    auto repoPath2 = tempFolder / "repo2";
    auto cloudPath1 = tempFolder / "cloud1";
    auto cloudPath2 = tempFolder / "cloud2";
    fs::create_directories(repoPath1);
    fs::create_directories(repoPath2);
    fs::create_directories(cloudPath1);
    fs::create_directories(cloudPath2);

    Settings settings{.useLocalCloud = true};
    Synchronizer synchronizer(*loop, settings, [](const Settings &s) {});
    CORRAL_WITH_NURSERY(n) {
      co_await n.start(
          [&synchronizer](corral::TaskStarted<> started) -> corral::Task<> {
            co_await synchronizer.run(std::move(started));
          });
      auto config1 = std::make_shared<LocalFolderConfig>();
      config1->folderPath = cloudPath1;
      auto config2 = std::make_shared<LocalFolderConfig>();
      config2->folderPath = cloudPath2;
      co_await synchronizer.addFolder(IUiGateProvider::Folder{
          .path = repoPath1, .cloudDestinations = {config1, config2}});
      co_await synchronizer.addFolder(IUiGateProvider::Folder{
          .path = repoPath2, .cloudDestinations = {config1, config2}});

      synchronizer.checkAllLocal();
      synchronizer.checkAllRemotes();
      co_await sleepFor(*loop, 200ms);

      // Everything is empty except for .hg folders
      EXPECT_EQ(filesInFolder(repoPath1, {".hg"}), 0);
      EXPECT_EQ(filesInFolder(repoPath2, {".hg"}), 0);
      EXPECT_EQ(filesInFolder(cloudPath1), 0);
      EXPECT_EQ(filesInFolder(cloudPath2), 0);

      // Now let's add a file to repoPath1 and see if it syncs to cloudPath1
      std::ofstream(repoPath1 / "file.txt") << "Hello, world!";
      synchronizer.checkAllLocal();
      TestConditionalWaiter waiter(*loop, [cloudPath1, cloudPath2]() {
        return filesInFolder(cloudPath1) >= 1 && filesInFolder(cloudPath2) >= 1;
      });
      EXPECT_TRUE(co_await waiter.waitFor(500ms));
      EXPECT_EQ(filesInFolder(cloudPath1), 1);
      EXPECT_EQ(filesInFolder(cloudPath2), 1);

      synchronizer.checkAllRemotes();
      TestConditionalWaiter waiter2(*loop, [repoPath2]() {
        return filesInFolder(repoPath2, {".hg"}) >= 1;
      });
      EXPECT_TRUE(co_await waiter2.waitFor(500ms));
      EXPECT_EQ(filesInFolder(repoPath2, {".hg"}), 1);

      // We need to give more time for rest commands to finish
      co_await sleepFor(*loop, 200ms);

      EXPECT_EQ(readAll(repoPath1 / "file.txt"),
                readAll(repoPath2 / "file.txt"));

      synchronizer.shutdown();
      co_return corral::join;
    };
  });
}
