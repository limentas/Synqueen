
#include "cloud/rclonecloudgate.hpp"
#include "gtestprinters.hpp"
#include "testutils.hpp"
#include "utils/continuationscheduler.hpp"
#include "utils/corraleventlooptraits.hpp"
#include "utils/standardpaths.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidjson/error/en.h>

using namespace testing;
using namespace std;
using namespace std::string_literals;
using namespace synqueen;
using namespace synqueen::cloud;
using namespace synqueen::utils;

class RcloneCloudGateTest : public testing::Test {
protected:
  void SetUp() override { StandardPaths::initializeInTemp("Synqueen-test"); }
  void TearDown() override { cleanupAppFolders(); }
};

TEST_F(RcloneCloudGateTest, Shutdown) {
  auto l = new uv_loop_t();
  auto result = uv_loop_init(l);
  EXPECT_EQ(result, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    RcloneCloudGate rcloneCloudGate(*loop);
    co_await rcloneCloudGate.shutdown();
    co_return;
  });
}

TEST_F(RcloneCloudGateTest, InitLocalFolderDestination) {
  auto l = new uv_loop_t();
  auto result = uv_loop_init(l);
  EXPECT_EQ(result, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    ContinuationScheduler::initialize(*loop);
    RcloneCloudGate rcloneCloudGate(*loop);
    RcloneFolderConfig config("id", StandardPaths::getTempPath());
    config.remoteType = RcloneFolderConfig::RemoteType::Local;
    config.useEncryption = true;
    co_await rcloneCloudGate.initDestination(config);
    co_await rcloneCloudGate.shutdown();
    co_return;
  });
}
