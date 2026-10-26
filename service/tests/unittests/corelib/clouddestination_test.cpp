#include "cloud/clouddestination.hpp"
#include "cloud/icloudgate.hpp"
#include "cloud/localfoldergate.hpp"
#include "ipatchstorage.hpp"
#include "testutils.hpp"
#include "utils/corraleventlooptraits.hpp"
#include "utils/corralheader.hpp"
#include "utils/standardpaths.hpp"
#include "utils/utils.hpp"
#include "utils/uvutils.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace testing;
using namespace std;
using namespace std::string_literals;
using namespace synqueen;
using namespace synqueen::utils;
using namespace synqueen::cloud;

namespace fs = std::filesystem;

class MockCloudGate : public ICloudGate {
public:
  MOCK_METHOD(corral::Task<FileDetailsByFileName>, listPatchFiles,
              (const CloudDestinationConfig &), (override));
  MOCK_METHOD(corral::Task<void>, uploadFiles,
              (const CloudDestinationConfig &,
               const std::list<std::filesystem::path> &),
              (override));
  MOCK_METHOD(corral::Task<std::list<std::filesystem::path>>, downloadFiles,
              (const CloudDestinationConfig &, const std::list<std::string> &,
               const std::filesystem::path &),
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

class CloudDestinationTest : public testing::Test {
protected:
  void SetUp() override { StandardPaths::initializeInTemp("Synqueen-test"); }
  void TearDown() override { cleanupAppFolders(); }
};

TEST_F(CloudDestinationTest, CtorDtor) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    CORRAL_WITH_NURSERY(n) {
      MockCloudGate mockGate;
      MockPatchStorage mockPatchStorage;
      EXPECT_CALL(mockPatchStorage, getOutgoingNotifier())
          .Times(AtLeast(1))
          .WillRepeatedly([]() -> utils::ISubscribable & {
            static Notifier notifier;
            return notifier;
          });
      CloudDestination cd(mockGate, LocalFolderConfig{}, mockPatchStorage, n);
      co_return corral::join;
    };
    co_return;
  });
}

// TODO: add more data to mock calls and figure out which mock calls should be
// uninterested and which should have expectations
TEST_F(CloudDestinationTest, Synchronize) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    CORRAL_WITH_NURSERY(n) {
      MockCloudGate mockGate;
      MockPatchStorage mockPatchStorage;
      EXPECT_CALL(mockPatchStorage, getOutgoingNotifier())
          .Times(AtLeast(1))
          .WillRepeatedly([]() -> utils::ISubscribable & {
            static Notifier notifier;
            return notifier;
          });

      CloudDestination cd(mockGate, LocalFolderConfig{}, mockPatchStorage, n);

      TestNotifiableWaiter waiter(*loop);
      EXPECT_CALL(mockGate, listPatchFiles(_))
          .Times(1)
          .WillOnce([]() -> corral::Task<FileDetailsByFileName> {
            return corral::just(FileDetailsByFileName{
                {"dummy_path",
                 FileDetails{.fileName = "dummy_path", .fileSize = 0}}});
          });
      EXPECT_CALL(mockPatchStorage, listOutgoingPatches())
          .Times(AtLeast(1))
          .WillOnce([]() -> const IPatchStorage::PatchesByFileName & {
            static const IPatchStorage::PatchesByFileName emptyMap{};
            return emptyMap;
          });
      EXPECT_CALL(mockPatchStorage, listPatchesToApply())
          .Times(AtLeast(1))
          .WillOnce([]() -> const IPatchStorage::PatchFilesByFileName & {
            static const IPatchStorage::PatchFilesByFileName emptyMap{};
            return emptyMap;
          });
      EXPECT_CALL(mockPatchStorage, listAppliedPatches())
          .Times(AtLeast(1))
          .WillOnce([]() -> const IPatchStorage::PatchFilesByFileName & {
            static const IPatchStorage::PatchFilesByFileName emptyMap{};
            return emptyMap;
          });
      EXPECT_CALL(mockGate,
                  downloadFiles(_, std::list<std::string>{"dummy_path"}, _))
          .Times(1)
          .WillOnce([]() -> corral::Task<std::list<std::filesystem::path>> {
            return corral::just(
                std::list<std::filesystem::path>{"dummy_path.patch"});
          });
      // The upload list is empty - it is fine to make the uploadFiles call
      EXPECT_CALL(mockGate, uploadFiles(_, std::list<std::filesystem::path>{}))
          .Times(AtMost(1))
          .WillOnce([]() -> corral::Task<void> { return corral::noop(); });
      EXPECT_CALL(mockPatchStorage,
                  movePatchesToApply(
                      std::list<std::filesystem::path>{"dummy_path.patch"}))
          .Times(1)
          .WillOnce([&waiter]() { waiter.notify(); });
      cd.synchronize();

      EXPECT_TRUE(co_await waiter.waitFor(500ms));

      co_return corral::join;
    };
    co_return;
  });
}
