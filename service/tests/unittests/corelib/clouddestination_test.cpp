#include "cloud/clouddestination.hpp"
#include "cloud/icloudgate.hpp"
#include "cloud/localfolder.hpp"
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
  MOCK_METHOD(corral::Task<FileDetailsList>, listPatchFiles,
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
  MOCK_METHOD(void, movePatchToOutgoing, (const std::filesystem::path &),
              (override));
  MOCK_METHOD((const std::map<int, IPatchStorage::PatchFile> &), listPatches,
              (), (const, override));
  MOCK_METHOD(utils::ISubscribable &, getIncomingNotifier, (), (override));
  MOCK_METHOD(utils::ISubscribable &, getOutgoingNotifier, (), (override));
};

TEST(CloudDestinationTest, CtorDtor) {
  StandardPaths::initialize("Synqueen-test");
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    auto tempFolder =
        co_await createTemporaryFolder("sq_test_XXXXXX", loop.get());

    {
      MockCloudGate mockGate;
      MockPatchStorage mockPatchStorage;
      CloudDestination cd(mockGate, LocalFolderConfig{}, mockPatchStorage);
      CORRAL_WITH_NURSERY(n) {
        cd.initialize(n);
        co_return corral::join;
      };
    }

    removeFileOrFolder(tempFolder);
    co_return;
  });

  // Cleanup the data folder after the test
  removeFileOrFolder(StandardPaths::getDataPath().string());
}

TEST(CloudDestinationTest, Synchronize) {
  StandardPaths::initialize("Synqueen-test");
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    auto tempFolder =
        co_await createTemporaryFolder("sq_test_XXXXXX", loop.get());

    {
      MockCloudGate mockGate;
      MockPatchStorage mockPatchStorage;
      CloudDestination cd(mockGate, LocalFolderConfig{}, mockPatchStorage);
      CORRAL_WITH_NURSERY(n) {
        cd.initialize(n);

        TestWaiter waiter(loop.get());
        Sequence s;
        EXPECT_CALL(mockGate, listPatchFiles(_))
            .Times(1)
            .InSequence(s)
            .WillOnce([]() -> corral::Task<FileDetailsList> {
              return corral::just(FileDetailsList{FileDetails{
                  .index = 1, .fileName = "dummy_path", .fileSize = 0}});
            });
        EXPECT_CALL(mockPatchStorage, listPatches())
            .Times(1)
            .InSequence(s)
            .WillOnce([]() -> const std::map<int, IPatchStorage::PatchFile> & {
              static const std::map<int, IPatchStorage::PatchFile> emptyMap{};
              return emptyMap;
            });
        EXPECT_CALL(mockGate,
                    downloadFiles(_, std::list<std::string>{"dummy_path"}, _))
            .Times(1)
            .InSequence(s)
            .WillOnce([]() -> corral::Task<std::list<std::filesystem::path>> {
              return corral::just(
                  std::list<std::filesystem::path>{"dummy_path.patch"});
            });
        EXPECT_CALL(mockPatchStorage,
                    movePatchesToApply(
                        std::list<std::filesystem::path>{"dummy_path.patch"}))
            .Times(1)
            .InSequence(s)
            .WillOnce([&waiter]() { waiter.notify(); });
        cd.synchronize();

        EXPECT_TRUE(co_await waiter.waitFor(500ms));

        co_return corral::join;
      };
    }

    removeFileOrFolder(tempFolder);
    co_return;
  });

  // Cleanup the data folder after the test
  removeFileOrFolder(StandardPaths::getDataPath().string());
}